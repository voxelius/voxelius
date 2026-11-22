#include "server/pch.hh"

#include "server/net/replication.hh"

#include "core/cvar/bits.hh"
#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/msg/capture.hh"
#include "core/msg/log.hh"
#include "core/utils/string.hh"

#include "shared/constant.hh"
#include "shared/net/packet_cvar.hh"
#include "shared/net/packet_msg.hh"
#include "shared/net/protocol.hh"

#include "server/globals.hh"
#include "server/net/sessions.hh"

constexpr static std::size_t MAX_CAPTURE = 32;

static cvar::Number<std::uint16_t> s_cmd_burst { 8, 1, 64 };
static cvar::Number<std::uint16_t> s_cmd_rate { 4, 1, 64 };

static cvar::status cmd_dbg_echo(const cvar::Context&, std::span<const std::string_view> args)
{
    msg::log_info("{}", utils::join(args, " "));

    return cvar::status::OK;
}

static void send_log_packet(const Session* session, std::uint32_t request_id, msg::level_type level, std::string_view message)
{
    packet::Msg_Log log {};
    log.request_id = request_id;
    log.level = level;
    log.message = std::string(message);

    protocol::send(log, session->peer);
}

static bool take_ratelimit_token(Session* session)
{
    if(session->cvar_ratelimit_tokens < 1.0f) {
        return false;
    }
    else {
        session->cvar_ratelimit_tokens -= 1.0f;
        return true;
    }
}

static void on_cvar_command(const packet::Cvar_Command& packet)
{
    auto session = sessions::find(packet.peer);

    if(session == nullptr || session->state != session_state::CONNECTED) {
        return;
    }

    if(take_ratelimit_token(session)) {
        cvar::Context ctx {};
        ctx.client_id = session->client_id;
        ctx.is_operator = session->access == session_access::OPERATOR;

        auto capture = msg::ScopedCapture(msg::LEVEL_INFO, MAX_CAPTURE);

        cvar::exec(packet.line, ctx);

        for(auto& record : capture.records()) {
            send_log_packet(session, packet.request_id, record.level, record.message);
        }
    }
    else {
        if(globals::current_tick - session->cvar_ratelimit_tick >= constant::FIXED_TICKRATE) {
            session->cvar_ratelimit_tick = globals::current_tick;

            send_log_packet(session, packet.request_id, msg::LEVEL_WARNING, "you are sending commands too quickly");
        }
    }
}

void replication::init(void)
{
    auto& net_cmd_burst_binding = cvar::bind_var(s_cmd_burst, "net_cmd_burst");
    net_cmd_burst_binding.set_help("How many console commands a client may send back to back");
    net_cmd_burst_binding.set_flags(FCVAR_ARCHIVE);

    auto& net_cmd_rate_binding = cvar::bind_var(s_cmd_rate, "net_cmd_rate");
    net_cmd_rate_binding.set_help("Console commands per second a client is allowed to sustain");
    net_cmd_rate_binding.set_flags(FCVAR_ARCHIVE);

    auto& dbg_echo_binding = cvar::bind_cmd("dbg_echo");
    dbg_echo_binding.set_help("Echo arguments back to the requesting client");
    dbg_echo_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN | FCVAR_HIDDEN);
    dbg_echo_binding.connect<&cmd_dbg_echo>();

    globals::dispatcher.sink<packet::Cvar_Command>().connect<&on_cvar_command>();
}

void replication::init_late(void)
{
    cvar::clear_dirty();
}

void replication::fixed_update_late(void)
{
    ZoneScoped;

    static packet::Cvar_ServerVars variables;

    auto refill = static_cast<float>(s_cmd_rate.value()) * globals::fixed_frametime;
    auto burst = static_cast<float>(s_cmd_burst.value());

    for(auto& session : sessions::all()) {
        if(session.state == session_state::UNCONNECTED) {
            continue;
        }

        session.cvar_ratelimit_tokens = std::min(session.cvar_ratelimit_tokens + refill, burst);
    }

    auto dirty = cvar::take_dirty();

    if(dirty.empty()) {
        return;
    }

    packet::Cvar_ServerVars::build_dirty(dirty, variables);

    if(variables.variables.empty()) {
        return;
    }

    for(auto& session : sessions::all()) {
        if(session.state == session_state::CONNECTED) {
            protocol::send(variables, session.peer);
        }
    }
}

void replication::synchronize(Session* session)
{
    static packet::Cvar_ServerVars variables;
    static packet::Cvar_ServerCmds commands;

    assert(session);

    packet::Cvar_ServerVars::build_full(variables);
    packet::Cvar_ServerCmds::build_full(commands, session->access == session_access::OPERATOR);

    protocol::send(variables, session->peer);
    protocol::send(commands, session->peer);
}
