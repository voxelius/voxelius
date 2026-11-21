#include "server/pch.hh"

#include "server/net/transmit.hh"

#include "shared/constant.hh"
#include "shared/cvar/server.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/world/weather.hh"
#include "shared/world/world.hh"
#include "shared/world/world_clock.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/sessions.hh"

constexpr static std::uint64_t WEATHER_SYNC_TICKS = constant::FIXED_TICKRATE;

static void on_block_update(const BlockUpdateEvent& event)
{
    packet::World_SetBlock packet {};
    packet.bpos = event.bpos();
    packet.block = event.id();
    interest::broadcast(event.world(), packet, event.cpos());
}

void transmit::init(void)
{
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
}

void transmit::fixed_update_late(void)
{
    packet::World_Timings timings {};
    timings.current_tick = globals::current_tick;
    timings.daylight_tick = globals::daylight_tick;
    timings.day = world_clock::day(globals::daylight_tick, server_vars::day_length_ticks());
    protocol::broadcast(timings, globals::host);

    // One small packet per second keeps both sides on the same front;
    // the seeds and scales travel once in World_Info, so only the
    // drift and the wind have to be refreshed here
    if(0 == (globals::current_tick % WEATHER_SYNC_TICKS)) {
        for(auto& session : sessions::all()) {
            if(session.state != session_state::CONNECTED || session.world == nullptr) {
                continue;
            }

            auto& state = session.world->weather();

            packet::World_Weather weather_state {};
            state.fill_packet(weather_state);
            protocol::send(weather_state, session.peer);
        }
    }
}
