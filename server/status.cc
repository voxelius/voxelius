// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <shared/protocol/packets/handshake.hh>
#include <shared/protocol/packets/server_info.hh>
#include <server/globals.hh>
#include <server/session.hh>
#include <server/status.hh>

static void on_handshake(const protocol::Handshake &packet)
{
    if(packet.intention == protocol::Handshake::PING) {
        protocol::ServerInfo response = {};
        response.version = protocol::VERSION;
        response.max_players = Session::max_players;
        response.num_players = 0; // FIXME
        response.flags = 0; // FIXME
        
        PacketWriter writer = {};
        PacketWriter::UI16(writer, protocol::ServerInfo::ID);
        protocol::encode_packet(writer, response);
        
        enet_peer_send(packet.peer, 0, PacketWriter::submit(writer, ENET_PACKET_FLAG_RELIABLE));
        
        return;
    }
}

void status::init(void)
{
    globals::dispatcher.sink<protocol::Handshake>().connect<&on_handshake>();
}
