#include "server/pch.hh"

#include "server/net/transmit.hh"

#include "shared/constant.hh"
#include "shared/net/packet_weather.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/world/weather.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"

constexpr static std::uint64_t WEATHER_SYNC_TICKS = constant::FIXED_TICKRATE;

static void on_block_update(const BlockUpdateEvent& event)
{
    packet::World_SetBlock packet {};
    packet.bpos = event.bpos();
    packet.block = event.id();
    interest::broadcast(packet, event.cpos());
}

void transmit::init(void)
{
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
}

void transmit::fixed_update_late(void)
{
    packet::World_Timings timings {};
    timings.current_tick = globals::current_tick;
    protocol::broadcast(timings, globals::host);

    // One small packet per second keeps both sides on the same front;
    // there is nothing to delta because the whole state is four fixed records
    if(0 == (globals::current_tick % WEATHER_SYNC_TICKS)) {
        packet::Weather_RealmState weather_state {};
        weather::fill_packet(weather_state);
        protocol::broadcast(weather_state, globals::host);
    }
}
