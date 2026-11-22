#include "client/pch.hh"

#include "client/net/receive.hh"

#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
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

#include "client/camera.hh"
#include "client/game.hh"
#include "client/globals.hh"
#include "client/net/session.hh"
#include "client/world/cloud_renderer.hh"
#include "client/world/skybox.hh"

static void on_entity_spawn(const packet::Entity_Spawn& packet)
{
    auto& registry = globals::world->registry();
    auto entity = utils::spawn(registry, packet.class_id, packet.entity);

    if(!registry.valid(entity)) {
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

    auto& registry = globals::world->registry();

    for(auto& it : packet.components) {
        buffer.reset(it.data);

        component_map::decode_net(registry, it.id, packet.entity, buffer);
    }

    msg::log_info("entity patch: id={} components={}", static_cast<std::uint64_t>(packet.entity), packet.components.size());
}

static void on_entity_remove(const packet::Entity_Remove& packet)
{
    auto& registry = globals::world->registry();

    msg::log_info("entity remove: id={}", static_cast<std::uint64_t>(packet.entity));

    registry.destroy(packet.entity);

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

    globals::dispatcher.trigger(ChunkUpdateEvent(globals::world, packet.cpos, chunk));
}

static void on_world_biomes(const packet::World_Biomes& packet)
{
    globals::world->insert_biomes(packet.cpos, packet.biomes);
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

        globals::dispatcher.trigger(ChunkLightUpdateEvent(globals::world, entry.cpos));
    }
}

static void on_world_set_block(const packet::World_SetBlock& packet)
{
    globals::world->set_block(packet.bpos, packet.block);
}

static void on_world_timings(const packet::World_Timings& packet)
{
    auto previous_tick = globals::world->daylight_tick;
    auto jump = packet.daylight_tick - previous_tick;

    if(previous_tick > packet.daylight_tick) {
        jump = previous_tick - packet.daylight_tick;
    }

    auto previous_weather_tick = globals::world->weather_tick;
    auto weather_jump = packet.weather_tick - previous_weather_tick;

    if(previous_weather_tick > packet.weather_tick) {
        weather_jump = previous_weather_tick - packet.weather_tick;
    }

    globals::current_tick = packet.current_tick;
    globals::world->daylight_tick = packet.daylight_tick;
    globals::world->weather_tick = packet.weather_tick;

    // A jump of a second or more is a deliberate time_set; the
    // ordinary one-tick step is left for the sky to interpolate
    if(jump > constant::FIXED_TICKRATE) {
        skybox::snap();
    }

    // The weather clock only jumps at a forced front, a rewind or a world
    // switch; the deck is rebuilt for those instead of sliding to the field
    if(weather_jump > constant::FIXED_TICKRATE) {
        cloud_renderer::reset();
    }
}

static void on_world_info(const packet::World_Info& packet)
{
    client_game::open_world(packet);

    cloud_renderer::reset();
    skybox::set_star_seed(weather::star_seed(packet.weather_seed));
    skybox::snap();
}

static cvar::status cmd_time_query(const cvar::Context&, std::span<const std::string_view>)
{
    auto day_length = server_vars::day_length_ticks();

    auto phase = world_clock::phase(globals::world->daylight_tick, day_length);
    auto daylight = world_clock::daylight(phase);
    auto day = world_clock::day(globals::world->daylight_tick, day_length);

    msg::log_info("time: tick={} daylight_tick={} day={} phase={:.3f} daylight={:.2f}", globals::current_tick,
        globals::world->daylight_tick, day, phase, daylight);

    return cvar::status::OK;
}

static cvar::status cmd_weather_status(const cvar::Context&, std::span<const std::string_view>)
{
    auto position = utils::to_fvec(camera::chunk) + camera::local;

    auto& definition = globals::world->definition();
    auto& state = globals::world->weather();
    auto& allowed = definition.weather;

    auto type = state.type_at(allowed, position.x(), position.z(), globals::world->weather_tick);
    auto coverage = state.coverage(allowed, position.x(), position.z(), globals::world->weather_tick);

    std::string description = "none";

    if(auto name = weather_registry::name_of(type)) {
        // The identifier is a temporary, so its view must
        // be copied before the optional goes out of scope
        description = name->full_string();
    }

    auto bpos = utils::to_block(camera::chunk, camera::local.cast<LocalPos::value_type>());
    auto temperature = globals::world->get_temperature(bpos);
    auto temperature_base = globals::world->get_temperature_base(bpos);
    auto humidity = globals::world->get_humidity(bpos);
    auto humidity_base = globals::world->get_humidity_base(bpos);

    msg::log_info("weather: type={} coverage={:.2f} temperature={:.1f}C base={:.1f}C humidity={:.2f} base={:.2f} tick={}", description,
        coverage, temperature, temperature_base, humidity, humidity_base, globals::world->weather_tick);

    return cvar::status::OK;
}

static void on_session_state(const SessionStateEvent& event)
{
    if(SESSION_DISCONNECTED == event.state()) {
        globals::player = entt::null;
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

    globals::dispatcher.sink<SessionStateEvent>().connect<&on_session_state>();

    auto& status_binding = cvar::bind_cmd("weather_status");
    status_binding.set_help("Print the weather state the client currently has");
    status_binding.connect<&cmd_weather_status>();

    auto& time_binding = cvar::bind_cmd("time_query");
    time_binding.set_help("Print the world clock the client currently has");
    time_binding.connect<&cmd_time_query>();
}
