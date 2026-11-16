#include "client/pch.hh"

#include "client/net/replication.hh"

#include "core/cvar/binding.hh"
#include "core/cvar/bits.hh"
#include "core/cvar/host.hh"
#include "core/msg/log.hh"

#include "shared/net/packet_cvar.hh"
#include "shared/net/packet_msg.hh"
#include "shared/net/packet_session.hh"
#include "shared/net/protocol.hh"

#include "client/globals.hh"
#include "client/net/session.hh"

static std::uint32_t s_next_request_id = 1;

static bool accept_shadow(const packet::Cvar_ServerVars::Variable& variable)
{
    if(auto binding = cvar::find_var(variable.name)) {
        auto flags = binding->flags().value_or(FCVAR_NONE);

        if(!binding->is_remote() && 0 == (flags & FCVAR_REPLICATED)) {
            msg::log_warning("replication: server wants to set {} which is not replicated here, ignoring it", variable.name);

            return true;
        }

        if(!binding->set_shadow(variable.value)) {
            msg::log_error("replication: server value {} for {} was rejected by the client validator", variable.value, variable.name);

            return false;
        }

        return true;
    }

    auto& new_binding = cvar::detail::make_remote_var(variable.name);
    new_binding.set_flags(variable.flags);

    if(variable.help.size()) {
        new_binding.set_help(variable.help);
    }

    if(!new_binding.set_shadow(variable.value)) {
        msg::log_error("replication: server value {} for {} was rejected", variable.value, variable.name);

        return false;
    }

    return true;
}

static cvar::status on_remote_exec(std::string_view line)
{
    if(globals::peer && (session::state == SESSION_SPAWNING || session::state == SESSION_INGAME)) {
        packet::Cvar_Command packet {};
        packet.request_id = s_next_request_id;
        packet.line = std::string(line);

        s_next_request_id += 1;

        if(s_next_request_id == packet::Msg_Log::UNSOLICITED) {
            s_next_request_id = 1;
        }

        protocol::send(packet, globals::peer);

        return cvar::status::OK;
    }
    else {
        msg::log_warning("{}: not connected to a server", line);

        return cvar::status::UNKNOWN;
    }
}

static void on_cvar_server_vars(const packet::Cvar_ServerVars& packet)
{
    if(session::state != SESSION_SPAWNING && session::state != SESSION_INGAME) {
        return;
    }

    if(packet.full) {
        cvar::clear_remote();
    }

    for(auto& variable : packet.variables) {
        if(!accept_shadow(variable)) {
            session::disconnect(packet::Session_Disconnect::CVAR_REJECTED);
            return;
        }
    }
}

static void on_cvar_server_cmds(const packet::Cvar_ServerCmds& packet)
{
    if(session::state != SESSION_SPAWNING && session::state != SESSION_INGAME) {
        return;
    }

    for(auto& command : packet.commands) {
        auto binding = cvar::find_cmd(command.name);

        if(binding) {
            auto flags = binding->flags().value_or(FCVAR_NONE);

            if(0 == (flags & FCVAR_RECALL)) {
                msg::log_warning("{}: server tried to set non-recallable command", command.name);

                continue;
            }
        }

        auto& remote = cvar::detail::make_remote_cmd(command.name);

        if(binding == nullptr) {
            remote.set_flags(command.flags);

            if(command.help.size()) {
                remote.set_help(command.help);
            }
        }
    }

    cvar::remote_exec_type hook {};
    hook.connect<&on_remote_exec>();
    cvar::set_remote_exec(std::move(hook));
}

static void on_msg_log(const packet::Msg_Log& packet)
{
    msg::detail::log(packet.level, std::source_location::current(), std::string_view(packet.message));
}

void replication::init(void)
{
    globals::dispatcher.sink<packet::Cvar_ServerVars>().connect<&on_cvar_server_vars>();
    globals::dispatcher.sink<packet::Cvar_ServerCmds>().connect<&on_cvar_server_cmds>();
    globals::dispatcher.sink<packet::Msg_Log>().connect<&on_msg_log>();
}

void replication::clear(void)
{
    cvar::set_remote_exec({});
    cvar::clear_remote();

    s_next_request_id = 1;
}
