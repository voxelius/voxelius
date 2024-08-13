// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <shared/math/constexpr.hh>
#include <shared/protocol/packets/disconnect.hh>
#include <shared/protocol/packets/entity_angles.hh>
#include <shared/protocol/packets/entity_angvel.hh>
#include <shared/protocol/packets/entity_head.hh>
#include <shared/protocol/packets/entity_position.hh>
#include <shared/protocol/packets/entity_transform.hh>
#include <shared/protocol/packets/entity_velocity.hh>
#include <shared/protocol/packets/handshake.hh>
#include <shared/protocol/packets/login_start.hh>
#include <shared/protocol/protocol.hh>
#include <shared/config.hh>
#include <server/globals.hh>
#include <server/network.hh>
#include <server/session.hh>
#include <spdlog/spdlog.h>

unsigned int server_network::listen_port = protocol::DEFAULT_PORT;
unsigned int server_network::incoming_bandwidth = 0U;
unsigned int server_network::outgoing_bandwidth = 0U;

static void handle_packet(ENetPeer *peer, const ENetPacket *packet)
{
    PacketReader reader = {};
    PacketReader::setup(reader, packet);

    protocol::Disconnect disconnect = {};
    protocol::EntityAngles entity_angles = {};
    protocol::EntityAngVel entity_angvel = {};
    protocol::EntityHead entity_head = {};
    protocol::EntityPosition entity_position = {};
    protocol::EntityTransform entity_transform = {};
    protocol::EntityVelocity entity_velocity = {};
    protocol::Handshake handshake = {};
    protocol::LoginStart login_start = {};

    switch(PacketReader::UI16(reader)) {
        case protocol::Disconnect::ID:
            disconnect.peer = peer;
            protocol::decode_packet(reader, disconnect);
            globals::dispatcher.trigger(disconnect);
            break;
        case protocol::EntityAngles::ID:
            entity_angles.peer = peer;
            protocol::decode_packet(reader, entity_angles);
            globals::dispatcher.trigger(entity_angles);
            break;
        case protocol::EntityAngVel::ID:
            entity_angvel.peer = peer;
            protocol::decode_packet(reader, entity_angvel);
            globals::dispatcher.trigger(entity_angvel);
            break;
        case protocol::EntityHead::ID:
            entity_head.peer = peer;
            protocol::decode_packet(reader, entity_head);
            globals::dispatcher.trigger(entity_head);
            break;
        case protocol::EntityPosition::ID:
            entity_position.peer = peer;
            protocol::decode_packet(reader, entity_position);
            globals::dispatcher.trigger(entity_position);
            break;
        case protocol::EntityTransform::ID:
            entity_transform.peer = peer;
            protocol::decode_packet(reader, entity_transform);
            globals::dispatcher.trigger(entity_transform);
            break;
        case protocol::EntityVelocity::ID:
            entity_velocity.peer = peer;
            protocol::decode_packet(reader, entity_velocity);
            globals::dispatcher.trigger(entity_velocity);
            break;
        case protocol::Handshake::ID:
            handshake.peer = peer;
            protocol::decode_packet(reader, handshake);
            globals::dispatcher.trigger(handshake);
            break;
        case protocol::LoginStart::ID:
            login_start.peer = peer;
            protocol::decode_packet(reader, login_start);
            globals::dispatcher.trigger(login_start);
            break;
    };
}

void server_network::init(void)
{
    Config::add(globals::server_config, "network.listen_port", server_network::listen_port);
    Config::add(globals::server_config, "network.incoming_bandwidth", server_network::incoming_bandwidth);
    Config::add(globals::server_config, "network.outgoing_bandwidth", server_network::outgoing_bandwidth);
}

void server_network::init_late(void)
{
    server_network::listen_port = cxpr::clamp<unsigned int>(server_network::listen_port, 1024U, UINT16_MAX);

    ENetAddress address = {};
    address.port = static_cast<enet_uint16>(server_network::listen_port);
    address.host = ENET_HOST_ANY;

    const auto max_peers = static_cast<enet_uint32>(Session::max_players + 1U);
    const auto bandwidth_in = static_cast<enet_uint32>(server_network::incoming_bandwidth);
    const auto bandwidth_out = static_cast<enet_uint32>(server_network::outgoing_bandwidth);
    globals::host = enet_host_create(&address, max_peers, 2, bandwidth_in, bandwidth_out);

    if(!globals::host) {
        spdlog::critical("network: unable to create host");
        std::terminate();
    }

    spdlog::info("network: listening at UDP port {}", server_network::listen_port);
}

void server_network::deinit(void)
{
    server_network::kick_all("Server Closed");
    enet_host_destroy(globals::host);
}

void server_network::update_late(void)
{
    ENetEvent event = {};

    while(enet_host_service(globals::host, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            handle_packet(event.peer, event.packet);
            enet_packet_destroy(event.packet);
            continue;
        }
        
        if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
            Session::destroy(reinterpret_cast<Session *>(event.peer->data));
            continue;
        }
    }
}

void server_network::kick_all(const std::string &reason)
{
    for(std::uint16_t i = 0; i < Session::max_players; ++i) {
        if(Session *session = Session::find(i)) {
            server_network::kick(session, reason);
            continue;
        }
    }
}

void server_network::kick(Session *session, const std::string &reason)
{
    protocol::Disconnect packet = {};
    packet.reason = reason;

    PacketWriter writer = {};
    protocol::encode_packet(writer, packet);

    enet_peer_send(session->peer, 0, PacketWriter::submit(writer, ENET_PACKET_FLAG_RELIABLE));
    enet_peer_disconnect(session->peer, 0);
}
