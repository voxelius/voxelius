#include "server/pch.hh"

#include "server/world/block_ticks.hh"

#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block.hh"
#include "shared/world/world.hh"

#include "server/world/world.hh"

static void on_block_update(const BlockUpdateEvent& event)
{
    // Only the server connects this handler, and every server world is a ServerWorld
    auto world = static_cast<ServerWorld*>(event.world());

    const std::array neighbours = {
        BlockPos(+1, 0, 0),
        BlockPos(-1, 0, 0),
        BlockPos(0, +1, 0),
        BlockPos(0, -1, 0),
        BlockPos(0, 0, +1),
        BlockPos(0, 0, -1),
    };

    // Schedule for the next tick so neighbour updates never fire mid-pass
    // in an order that depends on chunk iteration order.
    auto deadline = globals::current_tick + 1;

    for(const auto& npos : neighbours) {
        auto nbpos = event.bpos() + npos;
        auto ncpos = utils::to_chunk(nbpos);

        if(world->find_chunk(ncpos)) {
            world->schedule(nbpos, deadline, BLOCK_TICK_NEIGHBOUR);
        }
    }
}

void block_ticks::init(void)
{
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
}
