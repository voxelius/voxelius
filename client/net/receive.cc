#include "client/pch.hh"

#include "client/net/receive.hh"

#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/identifier.hh"
#include "core/msg/log.hh"

#include "shared/constant.hh"
#include "shared/cvar/server.hh"
#include "shared/entity/class_registry.hh"
#include "shared/entity/component_map.hh"
#include "shared/globals.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/packet_session.hh"
#include "shared/net/packet_world.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/entity.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_clock.hh"
#include "shared/world/world_registry.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/net/session.hh"
#include "client/world/cloud_renderer.hh"
#include "client/world/skybox.hh"

// The first weather packet of a connection sets the whole drift;
// later packets only correct it, so the deck does not slide on join
static bool s_weather_synced;

static void on_entity_spawn(const packet::Entity_Spawn& packet)
{
    auto entity = utils::spawn(packet.class_id, packet.entity);

    if(!globals::registry.valid(entity)) {
        session::disconnect(packet::Session_Disconnect::ENTITY_ID_DESYNC);
        return;
    }

    if(entity != packet.entity) {
        session::disconnect(packet::Session_Disconnect::ENTITY_ID_DESYNC);
        return;
    }

    auto class_name = class_registry::name_of(packet.class_id);

    if(class_name.has_value()) {
        msg::log_info("entity spawn: id={} class={}", static_cast<std::uint64_t>(entity), class_name->full_string());
    }
    else {
        msg::log_info("entity spawn: id={} class=<null>", static_cast<std::uint64_t>(entity));
    }
}

static void on_entity_patch(const packet::Entity_Patch& packet)
{
    static ReadBuffer buffer;

    for(auto& it : packet.components) {
        buffer.reset(it.data);

        component_map::decode_net(it.id, packet.entity, buffer);
    }

    msg::log_info("entity patch: id={} components={}", static_cast<std::uint64_t>(packet.entity), packet.components.size());
}

static void on_entity_remove(const packet::Entity_Remove& packet)
{
    msg::log_info("entity remove: id={}", static_cast<std::uint64_t>(packet.entity));

    globals::registry.destroy(packet.entity);

    if(globals::player == packet.entity) {
        globals::player = entt::null;

        session::disconnect(packet::Session_Disconnect::ENTITY_ID_DESYNC); // We can't really recover from this
    }
}

static void on_entity_client(const packet::Entity_Client& packet)
{
    msg::log_info("assign entity {} as a local player", static_cast<std::uint64_t>(packet.entity));

    globals::player = packet.entity;

    session::notify_spawned();
}

static void on_world_blocks(const packet::World_Blocks& packet)
{
    // A full chunk packet is authoritative: it is both the first load and the
    // resync the server sends to roll a stale client world back. Block edits
    // made after the packet was queued arrive as SetBlock on the same reliable
    // channel, so applying the whole chunk here cannot win over a newer prediction
    auto chunk = globals::world->find_chunk(packet.cpos);

    if(chunk == nullptr) {
        chunk = globals::world->create_chunk(packet.cpos);
    }

    chunk->set_blocks(packet.blocks);

    if(packet.light_valid) {
        chunk->set_light(packet.light);
    }
    else {
        chunk->clear_light();
    }

    globals::dispatcher.trigger(ChunkUpdateEvent(globals::world.get(), packet.cpos, chunk));
}

static void on_world_biomes(const packet::World_Biomes& packet)
{
    globals::world->insert_biomes(packet.realm, packet.cpos, packet.biomes);
}

static void on_world_light(const packet::World_Light& packet)
{
    for(const auto& entry : packet.entries) {
        auto chunk = globals::world->find_chunk(entry.cpos);

        if(chunk == nullptr) {
            continue;
        }

        if(entry.full) {
            chunk->set_light(entry.light);
        }
        else {
            // A delta cannot seed a chunk the client was never
            // given light, so the server sends the whole array in that case
            if(!chunk->has_light()) {
                continue;
            }

            for(std::size_t i = 0; i < entry.indices.size(); ++i) {
                chunk->set_packed_light(entry.indices[i], entry.values[i]);
            }
        }

        globals::dispatcher.trigger(ChunkLightUpdateEvent(globals::world.get(), entry.cpos));
    }
}

static void on_world_set_block(const packet::World_SetBlock& packet)
{
    globals::world->set_block(packet.bpos, packet.block);
}

static void on_world_timings(const packet::World_Timings& packet)
{
    auto previous_tick = globals::daylight_tick;
    auto jump = packet.daylight_tick > previous_tick ? packet.daylight_tick - previous_tick : previous_tick - packet.daylight_tick;

    globals::current_tick = packet.current_tick;
    globals::daylight_tick = packet.daylight_tick;
    globals::day = packet.day;

    // A jump of a second or more is a deliberate time_set; the
    // ordinary one-tick step is left for the sky to interpolate
    if(jump > constant::FIXED_TICKRATE) {
        skybox::snap();
    }
}

static void on_world_info(const packet::World_Info& packet)
{
    auto id = Identifier::from_string(packet.id);
    auto definition = world_registry::find(id);

    if(!definition) {
        session::disconnect(packet::Session_Disconnect::WORLD_UNKNOWN);
        return;
    }

    if(!globals::world || globals::world->id() != id) {
        globals::world = std::make_shared<World>(definition);
    }

    globals::world->seed = packet.seed;

    weather::apply_info(packet);

    skybox::set_star_seed(packet.star_seed);
}

static void on_world_weather(const packet::World_Weather& packet)
{
    // The first packet after connecting replaces the drift whole, so
    // the deck that was built from the old field must not slide to it
    auto baseline = !s_weather_synced;
    s_weather_synced = true;

    weather::apply_packet(packet, baseline);

    if(baseline) {
        cloud_renderer::reset();
    }
}

static cvar::status cmd_time_query(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    auto phase = world_clock::phase(globals::daylight_tick, server_vars::day_length_ticks());
    auto daylight = world_clock::daylight(phase);

    msg::log_info("time: tick={} daylight_tick={} day={} phase={:.3f} daylight={:.2f}", globals::current_tick, globals::daylight_tick,
        globals::day, phase, daylight);

    return cvar::status::OK;
}

static cvar::status cmd_weather_status(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    auto world = utils::to_fvec(camera::chunk) + camera::local;

    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        auto realm = static_cast<biome_realm>(i);
        auto& state = weather::state(realm);

        auto type = weather::type_at(realm, world.x(), world.z());
        auto coverage = weather::coverage(realm, world.x(), world.z());

        std::string description = "none";

        if(auto name = weather_registry::name_of(type)) {
            // The identifier is a temporary, so its view must be copied
            // before the optional goes out of scope
            description = name->full_string();
        }

        auto types = std::format("({:.0f}, {:.0f})/{}", state.types.drift.x(), state.types.drift.y(), state.types.scale);
        auto detail = std::format("({:.0f}, {:.0f})/{}", state.detail.drift.x(), state.detail.drift.y(), state.detail.scale);

        msg::log_info("weather[{}]: type={} coverage={:.2f} types={} detail={}", i, description, coverage, types, detail);
    }

    return cvar::status::OK;
}

static void on_session_state(const SessionStateEvent& event)
{
    if(SESSION_DISCONNECTED == event.state()) {
        globals::player = entt::null;
        s_weather_synced = false;
    }

    if(SESSION_AUTHENTICATING == event.state()) {
        s_weather_synced = false;
    }
}

void receive::init(void)
{
    globals::dispatcher.sink<packet::Entity_Spawn>().connect<&on_entity_spawn>();
    globals::dispatcher.sink<packet::Entity_Patch>().connect<&on_entity_patch>();
    globals::dispatcher.sink<packet::Entity_Remove>().connect<&on_entity_remove>();
    globals::dispatcher.sink<packet::Entity_Client>().connect<&on_entity_client>();

    globals::dispatcher.sink<packet::World_Blocks>().connect<&on_world_blocks>();
    globals::dispatcher.sink<packet::World_Biomes>().connect<&on_world_biomes>();
    globals::dispatcher.sink<packet::World_Light>().connect<&on_world_light>();
    globals::dispatcher.sink<packet::World_SetBlock>().connect<&on_world_set_block>();
    globals::dispatcher.sink<packet::World_Timings>().connect<&on_world_timings>();
    globals::dispatcher.sink<packet::World_Info>().connect<&on_world_info>();
    globals::dispatcher.sink<packet::World_Weather>().connect<&on_world_weather>();

    globals::dispatcher.sink<SessionStateEvent>().connect<&on_session_state>();

    auto& status_binding = cvar::bind_cmd("weather_status");
    status_binding.set_help("Print the weather state the client currently has");
    status_binding.connect<&cmd_weather_status>();

    auto& time_binding = cvar::bind_cmd("time_query");
    time_binding.set_help("Print the world clock the client currently has");
    time_binding.connect<&cmd_time_query>();
}
