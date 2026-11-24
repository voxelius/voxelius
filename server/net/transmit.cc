#include "server/pch.hh"

#include "server/net/transmit.hh"

#include "shared/cvar/server.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/coord.hh"
#include "shared/world/world_clock.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/universe.hh"
#include "server/world/columns.hh"
#include "server/world/world.hh"

static void on_block_update(const BlockUpdateEvent& event)
{
    packet::World_SetBlock packet {};
    packet.bpos = event.bpos();
    packet.block = event.id();
    interest::broadcast(event.world(), packet, event.cpos());
}

static void on_column_update(const ColumnUpdateEvent& event)
{
    packet::World_Column packet {};
    packet.bpos = event.pos();
    packet.column = event.column();
    interest::broadcast(event.world(), packet, utils::to_chunk_xz(event.pos()));
}

void transmit::init(void)
{
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
    globals::dispatcher.sink<ColumnUpdateEvent>().connect<&on_column_update>();
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
