#include "server/pch.hh"

#include "server/entity/entity_loader.hh"

#include "core/buffer.hh"
#include "core/identifier.hh"

#include "shared/component/transform.hh"
#include "shared/entity/class_registry.hh"
#include "shared/entity/component_map.hh"
#include "shared/globals.hh"
#include "shared/utils/entity.hh"
#include "shared/world/world.hh"

#include "server/constant.hh"
#include "server/entity/zent_file.hh"
#include "server/utils/region.hh"

constexpr static std::size_t MAX_REGIONS = 32;

struct EntityRegionEntry final {
    explicit EntityRegionEntry(std::filesystem::path path);

    std::mutex io_mutex; // TODO: threaded loading like in chunk_loader
    ZentFile region_file;
    std::chrono::steady_clock::time_point last_used;
};

static vx::hash_map<RegionKey, std::shared_ptr<EntityRegionEntry>> s_cache;

static std::filesystem::path region_path(const World* world, const RegionPos& region)
{
    auto ux = std::bit_cast<std::uint32_t>(region.x());
    auto uy = std::bit_cast<std::uint32_t>(region.y());
    auto uz = std::bit_cast<std::uint32_t>(region.z());
    auto name = std::format("R{:08X}{:08X}{:08X}.zent", ux, uy, uz);
    return world->entity_dir() / name;
}

static std::shared_ptr<EntityRegionEntry> find_region(const World* world, const RegionPos& pos)
{
    RegionKey key {};
    key.world = world;
    key.pos = pos;

    auto it = s_cache.find(key);

    if(it == s_cache.cend()) {
        return nullptr;
    }

    return it->second;
}

static std::shared_ptr<EntityRegionEntry> get_region(const World* world, const RegionPos& pos)
{
    if(auto entry = find_region(world, pos)) {
        return entry;
    }

    RegionKey key {};
    key.world = world;
    key.pos = pos;

    auto entry = std::make_shared<EntityRegionEntry>(region_path(world, pos));
    entry->last_used = std::chrono::steady_clock::now();

    if(s_cache.size() > MAX_REGIONS) {
        auto oldest = s_cache.begin();
        auto current = s_cache.begin();

        while(current != s_cache.end()) {
            if(current->second->last_used < oldest->second->last_used) {
                oldest = current;
            }

            current = std::next(current);
        }

        s_cache.erase(oldest);
    }

    s_cache.insert_or_assign(RegionKey(key), std::shared_ptr(entry));

    return entry;
}

static void encode_entity(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    thread_local std::vector<component_id_type> components;

    auto& entity_class = registry.get<EntityClass>(entity);
    auto name = class_registry::name_of(entity_class.id);

    if(name.has_value()) {
        buffer.write<std::string_view>(name->full_string());
    }
    else {
        buffer.write<std::string_view>({});
    }

    components.clear();

    class_registry::for_each(entity_class.id, [&](component_id_type id) {
        components.push_back(id);
    });

    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(components.size()));

    for(auto id : components) {
        buffer.write<std::string_view>(component_map::name_of(id));

        WriteBuffer component_buffer;
        component_map::encode_dat(registry, id, entity, component_buffer);

        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(component_buffer.size()));
        buffer.write_bytes(component_buffer.take());
    }
}

static void decode_entity(entt::registry& registry, ReadBuffer& buffer)
{
    thread_local std::vector<std::pair<std::string, std::vector<std::byte>>> components;

    components.clear();

    auto class_name = buffer.read<std::string>();
    auto num_components = buffer.read<std::uint16_t>();

    for(std::size_t i = 0; i < num_components; ++i) {
        auto component_name = buffer.read<std::string>();
        auto data_size = buffer.read<std::uint32_t>();

        std::vector<std::byte> data;
        data.resize(data_size);
        buffer.read_bytes(data);

        components.emplace_back(std::move(component_name), std::move(data));
    }

    if(class_name.empty()) {
        return;
    }

    auto identifier = Identifier::from_string(class_name);
    auto class_id = class_registry::find(identifier);

    if(class_id == CLASS_ID_NULL) {
        return;
    }

    auto entity = utils::spawn(registry, class_id);

    if(entity == entt::null) {
        return;
    }

    for(const auto& it : components) {
        auto component_id = component_map::from_name(it.first);

        if(component_id == COMPONENT_ID_NULL) {
            continue;
        }

        ReadBuffer component_buffer;
        component_buffer.reset(it.second);

        component_map::decode_dat(registry, component_id, entity, component_buffer);
    }
}

EntityRegionEntry::EntityRegionEntry(std::filesystem::path path) : region_file(std::move(path))
{
    // empty
}

void entity_loader::save(World* world, const ChunkPos& pos, std::span<const entt::entity> entities)
{
    assert(world);

    auto& registry = world->registry();

    if(entities.empty()) {
        return;
    }

    auto region = utils::region_of(pos);
    auto slot = utils::region_slot(pos, region);

    auto entry = get_region(world, region);

    WriteBuffer buffer;
    buffer.write<std::uint32_t>(static_cast<std::uint32_t>(entities.size()));

    for(auto entity : entities) {
        encode_entity(registry, entity, buffer);
    }

    std::scoped_lock io_lock(entry->io_mutex);
    entry->region_file.write_slot(slot, buffer);
}

void entity_loader::load(World* world, const ChunkPos& pos)
{
    assert(world);

    auto& registry = world->registry();

    auto region = utils::region_of(pos);
    auto slot = utils::region_slot(pos, region);

    auto entry = get_region(world, region);

    ReadBuffer buffer;

    std::scoped_lock io_lock(entry->io_mutex);

    if(!entry->region_file.read_slot(slot, buffer)) {
        return;
    }

    auto num_entities = buffer.read<std::uint32_t>();

    for(std::size_t i = 0; i < num_entities; ++i) {
        decode_entity(registry, buffer);
    }
}
