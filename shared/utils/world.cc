#include "shared/pch.hh"

#include "shared/utils/world.hh"

#include "core/utils/crc64.hh"

#include "shared/physics/physics.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

static std::string_view callback_name(const BlockFamily* family)
{
    auto& name = family->name;
    return name.full_string();
}

static bool call_routine(lua_State* L, int argc, int retc, std::string_view debug_name)
{
    auto status = lua_pcall(L, argc, retc, 0);

    if(status == LUA_OK) {
        return true;
    }

    msg::log_error("{}: {}", debug_name, lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
}

bool utils::block_place(const physics::BlockHit& hit, entt::entity actor, block_id_type id)
{
    assert(hit.world);

    WorldScope scope(hit.world);

    auto world = hit.world;

    auto family = block_registry::find_family_of(id);
    auto pos = hit.block_pos + hit.normal.cast<BlockPos::value_type>();

    const BlockCallback* callback = nullptr;

    if(family) {
        callback = &family->on_place;
    }

    auto has_on_place = true;
    has_on_place = has_on_place && callback;
    has_on_place = has_on_place && callback->callback_ref != LUA_NOREF;
    has_on_place = has_on_place && callback->state;

    auto can_occupy = true;
    auto existing = world->get_block(pos);

    if(existing) {
        auto def = block_registry::find_definition(existing);
        can_occupy = def && def->replaceable;
    }

    if(!has_on_place && !can_occupy) {
        return false;
    }

    if(!has_on_place) {
        return world->set_block(pos, id);
    }

    auto L = callback->state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback->callback_ref);

    lua_pushinteger(L, static_cast<lua_Integer>(pos.x()));
    lua_pushinteger(L, static_cast<lua_Integer>(pos.y()));
    lua_pushinteger(L, static_cast<lua_Integer>(pos.z()));

    lua_newtable(L);
    lua_pushinteger(L, static_cast<lua_Integer>(hit.stem));
    lua_setfield(L, -2, "stem");
    lua_pushinteger(L, static_cast<lua_Integer>(hit.face));
    lua_setfield(L, -2, "face");
    lua_pushnumber(L, static_cast<lua_Number>(hit.normal.x()));
    lua_setfield(L, -2, "ni");
    lua_pushnumber(L, static_cast<lua_Number>(hit.normal.y()));
    lua_setfield(L, -2, "nj");
    lua_pushnumber(L, static_cast<lua_Number>(hit.normal.z()));
    lua_setfield(L, -2, "nk");
    lua_pushnumber(L, static_cast<lua_Number>(hit.point.x()));
    lua_setfield(L, -2, "lx");
    lua_pushnumber(L, static_cast<lua_Number>(hit.point.y()));
    lua_setfield(L, -2, "ly");
    lua_pushnumber(L, static_cast<lua_Number>(hit.point.z()));
    lua_setfield(L, -2, "lz");
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.x()));
    lua_setfield(L, -2, "bx");
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.y()));
    lua_setfield(L, -2, "by");
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.z()));
    lua_setfield(L, -2, "bz");
    lua_pushnumber(L, static_cast<lua_Number>(std::fmod(hit.point.x(), 1.0f)));
    lua_setfield(L, -2, "rx");
    lua_pushnumber(L, static_cast<lua_Number>(std::fmod(hit.point.y(), 1.0f)));
    lua_setfield(L, -2, "ry");
    lua_pushnumber(L, static_cast<lua_Number>(std::fmod(hit.point.z(), 1.0f)));
    lua_setfield(L, -2, "rz");

    lua_newtable(L);
    lua_pushinteger(L, static_cast<lua_Integer>(existing));
    lua_setfield(L, -2, "id");
    lua_pushinteger(L, static_cast<lua_Integer>(can_occupy));
    lua_setfield(L, -2, "replaceable");

    lua_pushinteger(L, static_cast<lua_Integer>(actor));

    if(!call_routine(L, 6, 1, callback_name(family))) {
        return false;
    }

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    if(lua_istable(L, -1)) {
        vx::hash_map<blockstate_key_type, blockstate_val_type> states;

        for(const auto& it : family->states) {
            states.insert_or_assign(blockstate_key_type(it.first), blockstate_val_type(it.second.default_value));
        }

        lua_pushnil(L);

        while(lua_next(L, -2)) {
            if(lua_isstring(L, -2) && lua_isstring(L, -1)) {
                std::size_t key_length;
                std::size_t val_length;
                const auto key_cstr = lua_tolstring(L, -2, &key_length);
                const auto val_cstr = lua_tolstring(L, -1, &val_length);

                auto key = static_cast<blockstate_key_type>(utils::crc64(std::span(key_cstr, key_length)));

                if(family->states.contains(key)) {
                    auto val = family->state_hash(std::string_view(val_cstr, val_length));

                    states.insert_or_assign(key, blockstate_val_type(val));
                }
            }

            lua_pop(L, 1);
        }

        id = block_registry::resolve_variant(family->stem_id, states);
    }

    return world->set_block(pos, id);
}

bool utils::block_break(const physics::BlockHit& hit, entt::entity actor)
{
    assert(hit.world);

    WorldScope scope(hit.world);

    auto world = hit.world;

    auto replace_block = BLOCK_ID_NULL; // TODO: ask biome subsystem for this
    auto family = block_registry::find_family_of(hit.id);

    if(family == nullptr) {
        return world->set_block(hit.block_pos, replace_block);
    }

    const auto& callback = family->on_break;

    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return world->set_block(hit.block_pos, replace_block);
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);

    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.x()));
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.y()));
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.z()));
    lua_pushinteger(L, static_cast<lua_Integer>(actor));

    auto keep = false;

    if(call_routine(L, 4, 1, callback_name(family))) {
        if(lua_isboolean(L, -1)) {
            keep = static_cast<bool>(lua_toboolean(L, -1));
        }

        lua_pop(L, 1);
    }

    if(keep) {
        return true;
    }

    return world->set_block(hit.block_pos, replace_block);
}

bool utils::block_interact(const physics::BlockHit& hit, entt::entity actor)
{
    assert(hit.world);

    WorldScope scope(hit.world);

    auto family = block_registry::find_family_of(hit.id);

    if(family == nullptr) {
        return false;
    }

    const auto& callback = family->on_interact;

    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return false;
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);

    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.x()));
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.y()));
    lua_pushinteger(L, static_cast<lua_Integer>(hit.block_pos.z()));

    lua_newtable(L);
    lua_pushinteger(L, static_cast<lua_Integer>(hit.face));
    lua_setfield(L, -2, "face");
    lua_pushnumber(L, static_cast<lua_Number>(hit.normal.x()));
    lua_setfield(L, -2, "ni");
    lua_pushnumber(L, static_cast<lua_Number>(hit.normal.y()));
    lua_setfield(L, -2, "nj");
    lua_pushnumber(L, static_cast<lua_Number>(hit.normal.z()));
    lua_setfield(L, -2, "nk");
    lua_pushnumber(L, static_cast<lua_Number>(hit.point.x()));
    lua_setfield(L, -2, "lx");
    lua_pushnumber(L, static_cast<lua_Number>(hit.point.y()));
    lua_setfield(L, -2, "ly");
    lua_pushnumber(L, static_cast<lua_Number>(hit.point.z()));
    lua_setfield(L, -2, "lz");
    lua_pushinteger(L, static_cast<lua_Integer>(std::fmod(hit.point.x(), 1.0f)));
    lua_setfield(L, -2, "rx");
    lua_pushinteger(L, static_cast<lua_Integer>(std::fmod(hit.point.y(), 1.0f)));
    lua_setfield(L, -2, "ry");
    lua_pushinteger(L, static_cast<lua_Integer>(std::fmod(hit.point.z(), 1.0f)));
    lua_setfield(L, -2, "rz");

    lua_pushinteger(L, static_cast<lua_Integer>(actor));

    return call_routine(L, 5, 1, callback_name(family));
}

bool utils::block_sched_tick(const ChunkPos& cpos, const std::shared_ptr<Chunk>& chunk, std::size_t index, block_tick_source source)
{
    auto id = chunk->get_block(index);
    auto family = block_registry::find_family_of(id);

    if(family == nullptr) {
        return false;
    }

    const auto& callback = family->on_tick;

    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return false;
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);

    auto lpos = utils::to_local(index);
    auto bpos = utils::to_block(cpos, lpos);

    lua_pushinteger(L, static_cast<lua_Integer>(bpos.x()));
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.y()));
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.z()));

    lua_pushinteger(L, static_cast<lua_Integer>(source));

    return call_routine(L, 4, 1, callback_name(family));
}
