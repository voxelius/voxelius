#include "shared/pch.hh"

#include "shared/mod_context.hh"

#include "core/config/map.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"
#include "core/version.hh"

#include "shared/api/lua_libraries.hh"
#include "shared/constant.hh"

static std::pair<std::string, ModVersion> parse_mod_entry(std::string_view entry)
{
    auto separator = entry.find('@');

    if(separator == std::string_view::npos) {
        return std::make_pair(std::string(entry), ModVersion {});
    }

    auto name = std::string(entry.substr(0, separator));
    auto version = ModVersion::parse(entry.substr(separator + 1));

    return std::make_pair(std::move(name), std::move(version));
}

ModVersion ModVersion::parse(std::string_view string)
{
    ModVersion version {};
    std::string string_unfucked = std::string(string);
    auto check = std::sscanf(string_unfucked.c_str(), "%u.%u.%u", &version.major, &version.minor, &version.patch);

    if(check == 3) {
        return version;
    }

    return {};
}

std::string ModVersion::to_string(const ModVersion& version)
{
    return std::format("{}.{}.{}", version.major, version.minor, version.patch);
}

bool ModInfo::parse(const config::Map& map, ModInfo& modinfo)
{
    auto name = map.value<std::string>("name");

    if(!name.has_value() || name->empty()) {
        msg::log_error("missing required field 'name'");
        return false;
    }

    modinfo.name = std::move(name.value());
    modinfo.hard_depends.clear();
    modinfo.soft_depends.clear();
    modinfo.conflicts.clear();

    ModVersion builtin_version {};
    builtin_version.major = version::major;
    builtin_version.minor = version::minor;
    builtin_version.patch = version::patch;

    if(0 == modinfo.name.compare(constant::BUILTIN_NAME_SPACE)) {
        // Builtin modinfo.conf omits the version field
        // and instead uses whatever the runtime provides
        modinfo.version = builtin_version;
    }
    else {
        auto version_value = map.value<std::string>("version");
        modinfo.version = ModVersion::parse(version_value.value_or(std::string {}));

        // Any non-builtin mod implicitly hard-depends on builtin of the current runtime version
        modinfo.hard_depends.emplace_back(constant::BUILTIN_NAME_SPACE, builtin_version);
    }

    auto hard_depends_value = map.value<std::string>("hard_depends");
    auto hard_depends = hard_depends_value.value_or(std::string {});

    for(auto token : utils::tokenize(hard_depends)) {
        modinfo.hard_depends.emplace_back(parse_mod_entry(token));
    }

    auto soft_depends_value = map.value<std::string>("soft_depends");
    auto soft_depends = soft_depends_value.value_or(std::string {});

    for(auto token : utils::tokenize(soft_depends)) {
        modinfo.soft_depends.emplace_back(parse_mod_entry(token));
    }

    auto conflicts_value = map.value<std::string>("conflicts");
    auto conflicts = conflicts_value.value_or(std::string {});

    for(auto token : utils::tokenize(conflicts)) {
        modinfo.conflicts.emplace_back(parse_mod_entry(token));
    }

    auto meta_author_value = map.value<std::string>("meta_author");
    modinfo.meta_author = meta_author_value.value_or(std::string {});

    auto meta_homepage_value = map.value<std::string>("meta_homepage");
    modinfo.meta_homepage = meta_homepage_value.value_or(std::string {});

    auto meta_tracker_value = map.value<std::string>("meta_tracker");
    modinfo.meta_tracker = meta_tracker_value.value_or(std::string {});

    auto meta_license_value = map.value<std::string>("meta_license");
    modinfo.meta_license = meta_license_value.value_or(std::string("ARR"));

    auto display_name_value = map.value<std::string>("display_name");
    modinfo.display_name = display_name_value.value_or(modinfo.name);

    auto display_desc_value = map.value<std::string>("display_desc");
    modinfo.display_desc = display_desc_value.value_or(std::string {});

    return true;
}

ModContext::ModContext(ModInfo modinfo) : m_modinfo(std::move(modinfo)), m_status(mod_status::PENDING), m_lua_state(nullptr)
{
    // empty
}

void ModContext::set_status(mod_status status)
{
    m_status = status;
}

bool ModContext::initialize(std::span<const library_opener> libraries)
{
    assert(m_status == mod_status::PENDING);

    m_status = mod_status::LOADING;

    auto L = luaL_newstate();

    if(L == nullptr) {
        msg::log_error("{}: unable to create a Lua state", m_modinfo.name);
        m_status = mod_status::FAILED;
        return false;
    }

    m_lua_state = std::shared_ptr<lua_State>(L, &lua_close);
    api_shared::open_lua_libraries(m_lua_state, this);

    for(const auto& opener : libraries) {
        opener(m_lua_state, this);
    }

    auto entry = Identifier::from_parts(name_space(), "init.lua");
    auto entry_path = entry.as_file_path("scripts", {});
    auto chunk_name = std::format("@{}", entry_path);

    std::string source;

    if(!utils::read_file(entry_path, source)) {
        msg::log_error("{}: {}: {}", m_modinfo.name, entry_path, utils::physfs_error());
        m_status = mod_status::FAILED;
        return false;
    }

    auto load_status = luaL_loadbuffer(L, source.data(), source.size(), chunk_name.c_str());

    if(load_status != LUA_OK) {
        msg::log_error("{}: {}", m_modinfo.name, lua_tostring(L, -1));
        m_status = mod_status::FAILED;
        return false;
    }

    auto pcall_status = lua_pcall(L, 0, 0, 0);

    if(pcall_status != LUA_OK) {
        msg::log_error("{}: {}", m_modinfo.name, lua_tostring(L, -1));
        m_status = mod_status::FAILED;
        return false;
    }

    m_status = mod_status::LOADED;
    return true;
}

block_id_type ModContext::find_block(const Identifier& name) const
{
    auto it = m_block_names.find(name);

    if(it == m_block_names.cend())
        return BLOCK_ID_NULL;
    return it->second;
}

block_id_type ModContext::register_block(const Identifier& name, BlockDefinition def)
{
    if(m_blocks.empty()) {
        m_blocks.emplace_back();
    }

    auto id = static_cast<block_id_type>(m_blocks.size());

    m_blocks.push_back(std::move(def));
    m_block_names.try_emplace(name, id);

    return id;
}

block_family_id_type ModContext::register_block_family(BlockFamily family)
{
    if(m_block_families.empty()) {
        m_block_families.emplace_back();
    }

    auto id = static_cast<block_family_id_type>(m_block_families.size());

    m_block_families.push_back(std::move(family));

    return id;
}

bool ModContext::set_block_family(block_id_type id, block_family_id_type family)
{
    if(id == BLOCK_ID_NULL || id >= m_blocks.size()) {
        return false;
    }

    m_blocks[id].family = family;
    return true;
}

biome_id_type ModContext::find_biome(const Identifier& name) const
{
    auto it = m_biome_names.find(name);

    if(it == m_biome_names.cend())
        return BIOME_ID_NULL;
    return it->second;
}

biome_id_type ModContext::register_biome(const Identifier& name, BiomeDefinition def)
{
    if(m_biomes.empty()) {
        m_biomes.emplace_back();
    }

    auto id = static_cast<biome_id_type>(m_biomes.size());

    m_biomes.push_back(std::move(def));
    m_biome_names.try_emplace(name, id);

    return id;
}

std::vector<BlockDefinition> ModContext::take_blocks(void)
{
    return std::move(m_blocks);
}

std::vector<BlockFamily> ModContext::take_block_families(void)
{
    return std::move(m_block_families);
}

vx::hash_map<Identifier, block_id_type> ModContext::take_block_names(void)
{
    return std::move(m_block_names);
}

std::vector<BiomeDefinition> ModContext::take_biomes(void)
{
    return std::move(m_biomes);
}

vx::hash_map<Identifier, biome_id_type> ModContext::take_biome_names(void)
{
    return std::move(m_biome_names);
}

fluid_id_type ModContext::find_fluid(const Identifier& name) const
{
    auto it = m_fluid_names.find(name);

    if(it == m_fluid_names.cend())
        return FLUID_ID_NULL;
    return it->second;
}

fluid_id_type ModContext::register_fluid(const Identifier& name, FluidDefinition def)
{
    if(m_fluids.empty()) {
        m_fluids.emplace_back();
    }

    auto id = static_cast<fluid_id_type>(m_fluids.size());

    m_fluids.push_back(std::move(def));
    m_fluid_names.try_emplace(name, id);

    return id;
}

std::vector<FluidDefinition> ModContext::take_fluids(void)
{
    return std::move(m_fluids);
}

vx::hash_map<Identifier, fluid_id_type> ModContext::take_fluid_names(void)
{
    return std::move(m_fluid_names);
}

weather_type_id ModContext::find_weather(const Identifier& name) const
{
    auto it = m_weather_names.find(name);

    if(it == m_weather_names.cend())
        return WEATHER_ID_NULL;
    return it->second;
}

weather_type_id ModContext::register_weather(const Identifier& name, WeatherTypeDefinition def)
{
    if(m_weathers.empty()) {
        m_weathers.emplace_back();
    }

    auto id = static_cast<weather_type_id>(m_weathers.size());

    def.id = id;

    m_weathers.push_back(std::move(def));
    m_weather_names.try_emplace(name, id);

    return id;
}

std::vector<WeatherTypeDefinition> ModContext::take_weathers(void)
{
    return std::move(m_weathers);
}

vx::hash_map<Identifier, weather_type_id> ModContext::take_weather_names(void)
{
    return std::move(m_weather_names);
}

precip_type_id ModContext::find_precip(const Identifier& name) const
{
    auto it = m_precip_names.find(name);

    if(it == m_precip_names.cend())
        return PRECIP_ID_NULL;
    return it->second;
}

precip_type_id ModContext::register_precip(const Identifier& name, PrecipDefinition def)
{
    if(m_precips.empty()) {
        m_precips.emplace_back();
    }

    auto id = static_cast<precip_type_id>(m_precips.size());

    def.id = id;

    m_precips.push_back(std::move(def));
    m_precip_names.try_emplace(name, id);

    return id;
}

std::vector<PrecipDefinition> ModContext::take_precips(void)
{
    return std::move(m_precips);
}

vx::hash_map<Identifier, precip_type_id> ModContext::take_precip_names(void)
{
    return std::move(m_precip_names);
}

tint_id_type ModContext::find_tint(const Identifier& name) const
{
    auto it = m_tint_names.find(name);

    if(it == m_tint_names.cend())
        return TINT_ID_NULL;
    return it->second;
}

tint_id_type ModContext::register_tint(const Identifier& name, TintDefinition def)
{
    if(m_tints.empty()) {
        m_tints.emplace_back();
    }

    auto id = static_cast<tint_id_type>(m_tints.size());

    m_tints.push_back(std::move(def));
    m_tint_names.try_emplace(name, id);

    return id;
}

std::vector<TintDefinition> ModContext::take_tints(void)
{
    return std::move(m_tints);
}

vx::hash_map<Identifier, tint_id_type> ModContext::take_tint_names(void)
{
    return std::move(m_tint_names);
}

class_id_type ModContext::find_class(const Identifier& name) const
{
    auto it = m_class_names.find(name);

    if(it == m_class_names.cend())
        return CLASS_ID_NULL;
    return it->second;
}

class_id_type ModContext::register_class(const Identifier& name, ClassDefinition def)
{
    if(m_classes.empty()) {
        m_classes.emplace_back();
    }

    auto id = static_cast<class_id_type>(m_classes.size());

    m_classes.push_back(std::move(def));
    m_class_names.try_emplace(name, id);

    return id;
}

std::vector<ClassDefinition> ModContext::take_classes(void)
{
    return std::move(m_classes);
}

vx::hash_map<Identifier, class_id_type> ModContext::take_class_names(void)
{
    return std::move(m_class_names);
}
