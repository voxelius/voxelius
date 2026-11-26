#include "shared/pch.hh"

#include "shared/world/fluid_registry.hh"

#include "core/exception.hh"
#include "core/utils/crc64.hh"

#include "shared/mod_context.hh"

static std::vector<FluidDefinition> s_definitions;
static vx::hash_map<Identifier, fluid_id_type> s_names;
static vx::hash_map<fluid_id_type, Identifier> s_reverse_names;
static std::uint64_t s_checksum;

static void hash_trivial(std::uint64_t& hash, const void* data, std::size_t size)
{
    hash = utils::crc64(data, size, hash);
}

template<typename T>
static void hash_value(std::uint64_t& hash, const T& value)
{
    hash_trivial(hash, &value, sizeof(value));
}

static void hash_text(std::uint64_t& hash, std::string_view text)
{
    hash_value(hash, static_cast<std::uint64_t>(text.size()));
    hash_trivial(hash, text.data(), text.size());
}

static void update_checksum(void)
{
    auto hash = std::uint64_t { 0 };

    for(std::size_t i = 1; i < s_definitions.size(); ++i) {
        auto& def = s_definitions[i];
        auto name = s_reverse_names.find(static_cast<fluid_id_type>(i));

        if(name != s_reverse_names.cend()) {
            hash_text(hash, name->second.full_string());
        }

        hash_value(hash, static_cast<std::uint32_t>(def.gravity));
        hash_value(hash, static_cast<std::uint64_t>(def.albedo_still.size()));

        for(const auto& id : def.albedo_still) {
            hash_text(hash, id.full_string());
        }

        hash_value(hash, static_cast<std::uint64_t>(def.albedo_flowing.size()));

        for(const auto& id : def.albedo_flowing) {
            hash_text(hash, id.full_string());
        }

        hash_value(hash, static_cast<std::uint8_t>(def.mask_still.has_value()));

        if(def.mask_still) {
            hash_text(hash, def.mask_still->full_string());
        }

        hash_value(hash, static_cast<std::uint8_t>(def.mask_flowing.has_value()));

        if(def.mask_flowing) {
            hash_text(hash, def.mask_flowing->full_string());
        }

        hash_value(hash, static_cast<std::uint32_t>(def.full_level));
        hash_value(hash, def.physics.drag);
        hash_value(hash, def.physics.gravity_scale);
        hash_value(hash, def.physics.input_scale);
        hash_value(hash, def.physics.swim_speed);
        hash_value(hash, def.physics.climb_speed);
        hash_value(hash, def.physics.flow_scale);
        hash_value(hash, static_cast<std::uint8_t>(def.opaque));
        hash_value(hash, def.fog_density);
        hash_value(hash, def.fog_color);
        hash_text(hash, def.tint_name.full_string());
        hash_value(hash, static_cast<std::uint16_t>(def.tint));
    }

    s_checksum = hash;
}

std::span<const FluidDefinition> fluid_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t fluid_registry::checksum(void)
{
    return s_checksum;
}

void fluid_registry::resolve_tints(void)
{
    for(auto& def : s_definitions) {
        if(def.tint_name.is_empty()) {
            def.tint = TINT_ID_NULL;
            continue;
        }

        def.tint = tint_registry::find(def.tint_name);

        if(def.tint == TINT_ID_NULL) {
            msg::log_warning("fluid references unknown tint {}", def.tint_name.full_string());
        }
    }
}

void fluid_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto fluids = ctx.take_fluids();
    auto names = ctx.take_fluid_names();

    fluid_id_type fluid_offset;

    if(fluids.empty()) {
        fluid_offset = static_cast<fluid_id_type>(s_definitions.size());
    }
    else {
        fluid_offset = static_cast<fluid_id_type>(s_definitions.size()) - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + fluid_offset;
        auto result = s_names.try_emplace(name, global_id);

        if(!result.second) {
            msg::log_warning("duplicate fluid name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(fluids.size()) {
        s_definitions.insert(s_definitions.end(), std::make_move_iterator(fluids.begin() + 1), std::make_move_iterator(fluids.end()));
    }

    update_checksum();
}

void fluid_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();
    s_checksum = 0;
}

fluid_id_type fluid_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend())
        return FLUID_ID_NULL;
    return it->second;
}

std::optional<Identifier> fluid_registry::name_of(fluid_id_type id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const FluidDefinition* fluid_registry::find_definition(fluid_id_type id)
{
    if(id == FLUID_ID_NULL || id >= s_definitions.size())
        return nullptr;
    return &s_definitions[id];
}

const FluidDefinition* fluid_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}
