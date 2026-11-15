#include "server/pch.hh"

#include "server/net/host.hh"

#include "core/cmdline.hh"
#include "core/cvar/host.hh"
#include "core/cvar/string.hh"
#include "core/exception.hh"
#include "core/utils/enet.hh"

#include "shared/net/protocol.hh"

#include "server/globals.hh"
#include "server/net/sessions.hh"

cvar::Number<unsigned> host::max_players { 8, 1, 128 };

static cvar::Number<unsigned> s_status_peers { 4, 1, 16 };
static cvar::Number<unsigned short> s_port { protocol::PORT, 1024, 65535 };
static cvar::String s_bind { "0.0.0.0" };

void host::init(void)
{
    auto& sv_max_players_binding = cvar::bind_var(max_players, "sv_max_players");
    sv_max_players_binding.set_help("Maximum number of players");
    sv_max_players_binding.set_flags(FCVAR_ARCHIVE);

    auto& sv_status_peers_binding = cvar::bind_var(s_status_peers, "sv_status_peers");
    sv_status_peers_binding.set_help("Maximum number of status peers");
    sv_status_peers_binding.set_flags(FCVAR_ARCHIVE);

    auto& net_port_binding = cvar::bind_var(s_port, "net_port");
    net_port_binding.set_help("Listen port");
    net_port_binding.set_flags(FCVAR_ARCHIVE);

    auto& net_bind_binding = cvar::bind_var(s_bind, "net_bind");
    net_bind_binding.set_help("Address to bind the listen socket to");
    net_bind_binding.set_flags(FCVAR_ARCHIVE);
}

void host::init_late(void)
{
    auto max_players_v = max_players.value();
    max_players_v = std::max(max_players_v, 1U);
    max_players_v = std::min(max_players_v, 128U);

    auto status_peers = s_status_peers.value();
    status_peers = std::max(status_peers, 1U);
    status_peers = std::min(status_peers, 16U);

    auto listen_port = s_port.value();
    listen_port = std::max<std::uint16_t>(listen_port, 1024);

    max_players.set_value(max_players_v);
    s_status_peers.set_value(status_peers);
    s_port.set_value(listen_port);

    ENetAddress address {};
    address.host = ENET_HOST_ANY;
    address.port = s_port.value();
    enet_address_set_host(&address, s_bind.string().c_str());

    globals::host = enet_host_create(&address, max_players.value() + s_status_peers.value(), 1, 0, 0);
    vx::throw_if_not(globals::host, "enet_host_create failed");

    utils::enable_compression(globals::host);

    msg::log_info("listening on {}:{}", s_bind.value(), s_port.value());
    msg::log_info("max players: {}", max_players.value());
    msg::log_info("status peers: {}", s_status_peers.value());
}

void host::shutdown(void)
{
    enet_host_flush(globals::host);
    enet_host_service(globals::host, nullptr, 500);
    enet_host_destroy(globals::host);
}

void host::fixed_update_late(void)
{
    ENetEvent event {};

    while(0 < enet_host_service(globals::host, &event, 0)) {
        switch(event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                protocol::decode(globals::dispatcher, event.packet, event.peer);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                sessions::destroy(sessions::find(event.peer));
                break;
        }
    }
}
