#include "server/pch.hh"

#include "server/net/op_commands.hh"

#include "core/cvar/bits.hh"
#include "core/cvar/host.hh"
#include "core/msg/log.hh"

#include "shared/net/packet_session.hh"
#include "shared/net/protocol.hh"

#include "server/net/invites.hh"
#include "server/net/operators.hh"
#include "server/net/replication.hh"
#include "server/net/sessions.hh"

static void set_access(Session* session, session_access access)
{
    if(session->access == access) {
        return;
    }

    session->access = access;

    // The command list a client knows about depends on its access level.
    if(session->state == session_state::CONNECTED) {
        replication::synchronize(session);
    }

    if(access == session_access::OPERATOR) {
        msg::log_info("{} ({}) is now an operator", session->username, session->identity);
    }
    else {
        msg::log_info("{} ({}) is no longer an operator", session->username, session->identity);
    }
}

static Session* find_target(std::string_view name)
{
    std::uint16_t client_id;

    if(!name.empty() && name.size() <= 5) {
        auto check = std::from_chars(name.data(), name.data() + name.size(), client_id);

        if(check.ec == std::errc {} && check.ptr == name.data() + name.size()) {
            if(auto session = sessions::find(client_id)) {
                return session;
            }
        }
    }

    return sessions::find(name);
}

static cvar::status cmd_op_list(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    for(auto& session : sessions::all()) {
        if(session.state == session_state::UNCONNECTED) {
            continue;
        }

        auto access = session.access == session_access::OPERATOR ? "operator" : "player";

        msg::log_info("id={} user={} identity={} access={} pkey={}", session.client_id, session.username, session.identity, access,
            ed25519::export_pkey(session.pkey));
    }

    return cvar::status::OK;
}

static cvar::status cmd_op_add(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: op_add <public key>");

        return cvar::status::ARGUMENT;
    }

    auto pkey = ed25519::import_pkey(args[0]);

    if(!pkey.has_value()) {
        msg::log_warning("{}: not a valid public key", args[0]);

        return cvar::status::ARGUMENT;
    }

    operators::add(pkey.value());

    // Promote live sessions so that a new operator does not have to reconnect.
    for(auto& session : sessions::all()) {
        if(session.state != session_state::UNCONNECTED && session.pkey == pkey.value()) {
            set_access(&session, session_access::OPERATOR);
        }
    }

    msg::log_info("{}: added to operators.txt", args[0]);

    return cvar::status::OK;
}

static cvar::status cmd_op_remove(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: op_remove <public key>");

        return cvar::status::ARGUMENT;
    }

    auto pkey = ed25519::import_pkey(args[0]);

    if(!pkey.has_value()) {
        msg::log_warning("{}: not a valid public key", args[0]);

        return cvar::status::ARGUMENT;
    }

    operators::remove(pkey.value());

    for(auto& session : sessions::all()) {
        if(session.state != session_state::UNCONNECTED && session.pkey == pkey.value()) {
            set_access(&session, session_access::PLAYER);
        }
    }

    msg::log_info("{}: removed from operators.txt", args[0]);

    return cvar::status::OK;
}

static cvar::status cmd_op_kick(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: op_kick <client id or username>");

        return cvar::status::ARGUMENT;
    }

    auto target = find_target(args[0]);

    if(target == nullptr) {
        msg::log_warning("{}: not connected", args[0]);

        return cvar::status::ARGUMENT;
    }

    // The caller's own log reply would be sent to a session that no longer has a peer.
    if(ctx.client_id.has_value() && target->client_id == ctx.client_id.value()) {
        msg::log_warning("{}: you cannot kick yourself", target->username);

        return cvar::status::ARGUMENT;
    }

    msg::log_info("{} ({}) was kicked", target->username, target->identity);

    packet::Session_Disconnect response {};
    response.reason = packet::Session_Disconnect::KICKED;
    protocol::send(response, target->peer);

    enet_peer_disconnect_later(target->peer, 0);

    sessions::destroy(target, packet::Session_Disconnect::KICKED);

    return cvar::status::OK;
}

static cvar::status cmd_op_invite(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    msg::log_info("invite code: {}", invites::issue());

    return cvar::status::OK;
}

void op_commands::init(void)
{
    auto& list_binding = cvar::bind_cmd("op_list");
    list_binding.set_help("List connected sessions and their access levels");
    list_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    list_binding.connect<&cmd_op_list>();

    auto& add_binding = cvar::bind_cmd("op_add");
    add_binding.set_help("Add a public key to operators.txt and promote that player");
    add_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    add_binding.connect<&cmd_op_add>();

    auto& remove_binding = cvar::bind_cmd("op_remove");
    remove_binding.set_help("Remove a public key from operators.txt and demote that player");
    remove_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    remove_binding.connect<&cmd_op_remove>();

    auto& kick_binding = cvar::bind_cmd("op_kick");
    kick_binding.set_help("Disconnect a player by client ID or username");
    kick_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    kick_binding.connect<&cmd_op_kick>();

    auto& invite_binding = cvar::bind_cmd("op_invite");
    invite_binding.set_help("Issue a one-time invite code that whitelists whoever redeems it");
    invite_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    invite_binding.connect<&cmd_op_invite>();
}

void op_commands::shutdown(void)
{
    // empty
}
