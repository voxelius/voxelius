#include "client/pch.hh"

#include "client/net/transmit.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/net/packet_player.hh"
#include "shared/net/protocol.hh"
#include "shared/system/pmove.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"

void transmit::fixed_update_late(void)
{
    auto& registry = globals::world->registry();

    if(globals::peer && registry.valid(globals::player)) {
        const auto& head = registry.get<Head>(globals::player);
        const auto& transform = registry.get<Transform>(globals::player);
        const auto& velocity = registry.get<Velocity>(globals::player);

        packet::Player_MoveData packet {};
        packet.simulated_lpos = transform.local;
        packet.simulated_cpos = transform.chunk;
        packet.camera_angles.x() = head.angles.x();
        packet.camera_angles.y() = head.angles.y();
        packet.camera_angles.z() = head.angles.z();
        packet.velocity = velocity.value;

        if(const auto move_data = registry.try_get<MoveData>(globals::player)) {
            packet.wishdir = move_data->wishdir;
        }

        protocol::send(packet, globals::peer);
    }
}
