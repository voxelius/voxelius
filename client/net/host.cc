#include "client/pch.hh"

#include "client/net/host.hh"

#include "core/exception.hh"
#include "core/utils/enet.hh"

#include "shared/net/protocol.hh"

#include "client/globals.hh"

void host::init(void)
{
    globals::peer = nullptr;
    globals::host = enet_host_create(nullptr, 1, 1, 0, 0);
    vx::throw_if_not(globals::host, "enet_host_create failed");

    utils::enable_compression(globals::host);
}

void host::shutdown(void)
{
    if(globals::peer) {
        enet_host_flush(globals::host);
        enet_peer_reset(globals::peer);
    }

    enet_host_destroy(globals::host);

    globals::host = nullptr;
    globals::peer = nullptr;
}

void host::fixed_update_late(void)
{
    ENetEvent event {};

    while(0 < enet_host_service(globals::host, &event, 0)) {
        switch(event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                globals::dispatcher.trigger(HostConnectEvent(std::move(event)));
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                protocol::decode(globals::dispatcher, event.packet, event.peer);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                globals::peer = nullptr;
                globals::dispatcher.trigger(HostDisconnectEvent(std::move(event)));
                break;

            case ENET_EVENT_TYPE_NONE:
                break; // shut up clang
        }
    }
}

bool host::connect(std::string_view address, std::uint16_t port)
{
    if(globals::peer) {
        return false;
    }

    ENetAddress enet_address {};
    enet_address.port = port;

    auto address_str = std::string(address);

    if(enet_address_set_host(&enet_address, address_str.c_str()) < 0) {
        return false;
    }

    globals::peer = enet_host_connect(globals::host, &enet_address, 1, 0);

    return static_cast<bool>(globals::peer);
}

void host::disconnect(void)
{
    if(globals::peer) {
        if(globals::peer->state == ENET_PEER_STATE_CONNECTED) {
            enet_peer_disconnect_later(globals::peer, 0);
        }
        else {
            enet_peer_reset(globals::peer);
            globals::peer = nullptr;
        }
    }
}
