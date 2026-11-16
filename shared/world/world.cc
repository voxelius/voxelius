#include "shared/pch.hh"

#include "shared/world/world.hh"

#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/block_registry.hh"

vx::hash_map<ChunkPos, std::shared_ptr<Chunk>> world::chunk_map;
entt::registry world::chunk_registry;

ChunkCreateEvent::ChunkCreateEvent(const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk) : m_chunk(chunk), m_pos(pos)
{
    // empty
}

ChunkRemoveEvent::ChunkRemoveEvent(const ChunkPos& pos, const std::shared_ptr<const Chunk>& chunk) : m_chunk(chunk), m_pos(pos)
{
    // empty
}

ChunkUpdateEvent::ChunkUpdateEvent(const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk) : m_chunk(chunk), m_pos(pos)
{
    // empty
}

ChunkReadyEvent::ChunkReadyEvent(const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk) : m_chunk(chunk), m_pos(pos)
{
    // empty
}

ChunkLightUpdateEvent::ChunkLightUpdateEvent(const ChunkPos& pos) : m_pos(pos)
{
    // empty
}

BlockUpdateEvent::BlockUpdateEvent(const BlockPos& pos, block_id_type id, const std::shared_ptr<Chunk>& chunk)
    : m_chunk(chunk), m_id(id), m_bpos(pos), m_cpos(utils::to_chunk(pos)), m_lpos(utils::to_local(pos))
{
    // empty
}

std::shared_ptr<Chunk> world::create_chunk(const ChunkPos& pos)
{
    auto it = chunk_map.find(pos);

    if(it == chunk_map.cend()) {
        auto entity = chunk_registry.create();
        auto chunk = std::make_shared<Chunk>(entity);

        auto& component = chunk_registry.emplace<Chunk_Component>(entity);
        component.position = pos;
        component.ptr = chunk;

        chunk_map.insert_or_assign(ChunkPos(pos), std::shared_ptr(chunk));

        globals::dispatcher.trigger(ChunkCreateEvent(pos, chunk));

        return chunk;
    }

    return it->second;
}

std::shared_ptr<Chunk> world::find_chunk(const ChunkPos& pos)
{
    auto it = chunk_map.find(pos);

    if(it == chunk_map.cend()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<Chunk> world::find_chunk(entt::entity entity)
{
    if(chunk_registry.valid(entity)) {
        auto& component = chunk_registry.get<Chunk_Component>(entity);
        return component.ptr;
    }

    return nullptr;
}

void world::remove_chunk(const std::shared_ptr<const Chunk>& chunk)
{
    if(chunk) {
        remove_chunk(chunk->entity());
    }
}

void world::remove_chunk(const ChunkPos& pos)
{
    auto it = chunk_map.find(pos);

    if(it == chunk_map.cend()) {
        return;
    }

    globals::dispatcher.trigger(ChunkRemoveEvent(pos, it->second));

    chunk_registry.destroy(it->second->entity());
    chunk_map.erase(it);
}

void world::remove_chunk(entt::entity entity)
{
    if(chunk_registry.valid(entity)) {
        const auto& component = chunk_registry.get<Chunk_Component>(entity);

        globals::dispatcher.trigger(ChunkRemoveEvent(component.position, component.ptr));

        chunk_map.erase(component.position);
        chunk_registry.destroy(entity);
    }
}

block_id_type world::get_block(const ChunkPos& cpos, const LocalPos& lpos)
{
    if(auto chunk = find_chunk(cpos)) {
        return chunk->get_block(lpos);
    }

    return BLOCK_ID_NULL;
}

block_id_type world::get_block(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_block(cpos, lpos);
}

fluid_id_type world::get_fluid(const ChunkPos& cpos, const LocalPos& lpos)
{
    auto id = get_block(cpos, lpos);

    if(auto def = block_registry::find_definition(id)) {
        return def->fluid;
    }

    return FLUID_ID_NULL;
}

fluid_id_type world::get_fluid(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_fluid(cpos, lpos);
}

std::uint32_t world::get_fluid_faces(const ChunkPos& cpos, const LocalPos& lpos)
{
    auto id = get_block(cpos, lpos);

    if(auto def = block_registry::find_definition(id)) {
        return def->fluid_faces;
    }

    return 0;
}

std::uint32_t world::get_fluid_faces(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_fluid_faces(cpos, lpos);
}

fluid_id_type world::get_state_fluid(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value)
{
    auto id = get_block(cpos, lpos);
    auto def = block_registry::find_definition(id);

    if(def == nullptr) {
        return FLUID_ID_NULL;
    }

    auto family = block_registry::find_family(def->family);

    if(family == nullptr) {
        return FLUID_ID_NULL;
    }

    auto state_key = family->state_hash(state);

    if(!family->states.contains(state_key)) {
        return FLUID_ID_NULL;
    }

    auto value_key = family->state_hash(value);

    return block_registry::preview_fluid(id, state_key, value_key);
}

fluid_id_type world::get_state_fluid(const BlockPos& pos, std::string_view state, std::string_view value)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_state_fluid(cpos, lpos, state, value);
}

bool world::set_block(const ChunkPos& cpos, const LocalPos& lpos, block_id_type id)
{
    if(auto chunk = find_chunk(cpos)) {
        if(chunk->get_block(lpos) == id) {
            return true;
        }

        chunk->set_block(lpos, id);

        globals::dispatcher.trigger(BlockUpdateEvent(utils::to_block(cpos, lpos), id, chunk));

        const std::array neighbours = {
            BlockPos(+1, 0, 0),
            BlockPos(-1, 0, 0),
            BlockPos(0, +1, 0),
            BlockPos(0, -1, 0),
            BlockPos(0, 0, +1),
            BlockPos(0, 0, -1),
        };

        auto bpos = utils::to_block(cpos, lpos);

        // Schedule for the next tick so neighbour updates never fire mid-pass
        // in an order that depends on chunk iteration order.
        auto neighbour_deadline = globals::current_tick + 1;

        for(const auto& npos : neighbours) {
            auto nbpos = bpos + npos;
            auto ncpos = utils::to_chunk(nbpos);

            if(chunk_map.contains(ncpos)) {
                schedule(nbpos, neighbour_deadline, BLOCK_TICK_NEIGHBOUR);
            }
        }

        return true;
    }

    return false;
}

bool world::set_block(const BlockPos& pos, block_id_type id)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return set_block(cpos, lpos, id);
}

block_light_type world::get_light(const ChunkPos& cpos, const LocalPos& lpos)
{
    if(auto chunk = find_chunk(cpos)) {
        auto index = utils::to_index(lpos);
        auto sky = chunk->get_light(index, LIGHT_SKY);
        auto block = chunk->get_light(index, LIGHT_BLOCK);
        return std::max(sky, block);
    }

    return BLOCK_LIGHT_MIN;
}

block_light_type world::get_light(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_light(cpos, lpos);
}

std::optional<std::string_view> world::get_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state)
{
    auto id = get_block(cpos, lpos);

    if(id == BLOCK_ID_NULL) {
        return std::nullopt;
    }

    auto family = block_registry::find_family_of(id);

    if(family == nullptr) {
        return std::nullopt;
    }

    auto key = family->state_hash(state);
    auto decl_it = family->states.find(key);

    if(decl_it == family->states.cend()) {
        return std::nullopt; // this family has no such state
    }

    auto id_it = family->id_states.find(id);

    if(id_it == family->id_states.cend()) {
        return family->state_value(decl_it->second.default_value);
    }

    auto val_it = id_it->second.find(key);

    if(val_it == id_it->second.cend()) {
        return family->state_value(decl_it->second.default_value);
    }

    return family->state_value(val_it->second);
}

std::optional<std::string_view> world::get_state(const BlockPos& pos, std::string_view state)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_state(cpos, lpos, state);
}

bool world::set_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value)
{
    auto id = get_block(cpos, lpos);

    if(id == BLOCK_ID_NULL) {
        return false;
    }

    auto family = block_registry::find_family_of(id);

    if(family == nullptr) {
        return false;
    }

    auto key = family->state_hash(state);

    if(0 == family->states.count(key)) {
        return false;
    }

    vx::hash_map<blockstate_key_type, blockstate_val_type> chunk_map;

    for(const auto& it : family->states) {
        chunk_map.try_emplace(it.first, it.second.default_value);
    }

    auto id_it = family->id_states.find(id);

    if(id_it != family->id_states.cend()) {
        for(const auto& it : id_it->second) {
            chunk_map.insert_or_assign(blockstate_key_type(it.first), blockstate_val_type(it.second));
        }
    }

    chunk_map.insert_or_assign(key, family->state_hash(value));

    auto new_id = block_registry::resolve_variant(family->stem_id, chunk_map);

    return set_block(cpos, lpos, new_id);
}

bool world::set_state(const BlockPos& pos, std::string_view state, std::string_view value)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return set_state(cpos, lpos, state, value);
}

std::int32_t world::get_temperature_base(const ChunkPos& cpos, const LocalPos& lpos)
{
    return 298; // TODO
}

std::int32_t world::get_temperature_base(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_temperature_base(cpos, lpos);
}

std::int32_t world::get_temperature(const ChunkPos& cpos, const LocalPos& lpos)
{
    return 298; // TODO
}

std::int32_t world::get_temperature(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_temperature(cpos, lpos);
}

biome_id_type world::get_biome(const ChunkPos& cpos, const LocalPos& lpos)
{
    return biome_map::get_slow(utils::to_block(cpos, lpos));
}

biome_id_type world::get_biome(const BlockPos& pos)
{
    return biome_map::get_slow(pos);
}

void world::schedule(const ChunkPos& cpos, const LocalPos& lpos, std::uint64_t deadline, block_tick_source source)
{
    if(auto chunk = find_chunk(cpos)) {
        auto index = utils::to_index(lpos);

        chunk->schedule(index, deadline, source);
    }
}

void world::schedule(const BlockPos& pos, std::uint64_t deadline, block_tick_source source)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    schedule(cpos, lpos, deadline, source);
}

void world::purge(void)
{
    chunk_map.clear();
    chunk_registry.clear();

    biome_map::purge();
}

void world::fixed_update(void)
{
    std::vector<std::pair<std::size_t, block_tick_source>> due;

    chunk_registry.view<Chunk_Component>().each([&](Chunk_Component& component) {
        due.clear();

        component.ptr->pop_due(globals::current_tick, due);

        for(auto it : due) {
            utils::block_sched_tick(component.position, component.ptr, it.first, it.second);
        }
    });
}
