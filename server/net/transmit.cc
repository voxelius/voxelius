#include "server/pch.hh"

#include "server/net/transmit.hh"

#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"

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
}
