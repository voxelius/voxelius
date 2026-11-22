#include "server/pch.hh"

#include "server/net/transmit.hh"

#include "shared/cvar/server.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/world/world_clock.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/universe.hh"
#include "server/world/world.hh"

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
    // Each world owns its clocks, so every world gets its own packet;
    // interest only hands it to the sessions that are inside that world
    for(const auto& world : universe::worlds()) {
        packet::World_Timings timings {};
        timings.current_tick = globals::current_tick;
        timings.daylight_tick = world->daylight_tick;
        timings.day = world_clock::day(world->daylight_tick, server_vars::day_length_ticks());
        timings.weather_tick = world->weather_tick;
        timings.season_tick = 0; // Reserved for the season cycle

        interest::broadcast(world.get(), timings);
    }
}
