#include "server/pch.hh"

#include "server/net/transmit.hh"

#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/world/world.hh"

#include "core/msg/log.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"

// Five seconds at the fixed rate; long enough to read a rate off the numbers.
constexpr static std::uint64_t STATS_INTERVAL = 100;

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

    static std::uint64_t last_tick;

    if(globals::current_tick - last_tick < STATS_INTERVAL) {
        return;
    }

    last_tick = globals::current_tick;

    auto stats = protocol::take_sent_stats();

    msg::log_info("net sent: chunks={} biomes={} light={} setblock={} entities={} timings={} other={}", stats.chunks, stats.biomes,
        stats.light, stats.set_block, stats.entities, stats.timings, stats.other);
}
