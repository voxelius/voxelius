#include "server/pch.hh"

#include "server/net/status.hh"

#include "core/config/ref.hh"
#include "core/version.hh"

#include "shared/net/packet_status.hh"
#include "shared/net/protocol.hh"
#include "shared/splash.hh"

#include "server/globals.hh"
#include "server/net/host.hh"
#include "server/net/sessions.hh"
#include "server/net/whitelist.hh"

static void on_status_request(const packet::Status_Request& packet)
{
    std::uint32_t server_tags = 0;

    if(whitelist::enabled.value()) {
        server_tags |= packet::Status_Response::WHITELIST_ENABLED;
    }

    if(sessions::strict_version.value()) {
        server_tags |= packet::Status_Response::STRICT_VERSION;
    }

    packet::Status_Response response {};
    response.version_major = version::major;
    response.max_players = host::max_players.value();
    response.num_players = sessions::num_players;
    response.motd = splash::get();
    response.version_minor = version::minor;
    response.version_patch = version::patch;
    response.server_tags = server_tags;

    protocol::send(response, packet.peer);
}

void status::init(void)
{
    globals::dispatcher.sink<packet::Status_Request>().connect<&on_status_request>();
}
