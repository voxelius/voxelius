#include "server/pch.hh"

#include "server/world/time_sim.hh"

#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/msg/log.hh"

#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/world/world_clock.hh"

static cvar::status cmd_time_set(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.size() != 1) {
        msg::log_error("usage: time_set <daylight tick>");

        return cvar::status::ARGUMENT;
    }

    auto begin = args[0].data();
    auto end = begin + args[0].size();

    std::uint64_t tick;
    auto result = std::from_chars(begin, end, tick);

    if(result.ec != std::errc {} || result.ptr != end) {
        msg::log_error("time_set: not a tick count: {}", args[0]);

        return cvar::status::ARGUMENT;
    }

    globals::daylight_tick = tick;

    auto day_length = server_vars::day_length_ticks();
    auto phase = world_clock::phase(tick, day_length);
    auto day = world_clock::day(tick, day_length);

    msg::log_info("time_set: daylight_tick={} day={} phase={:.3f}", tick, day, phase);

    return cvar::status::OK;
}

void time_sim::init(void)
{
    auto& time_set_binding = cvar::bind_cmd("time_set");
    time_set_binding.set_help("Set the world clock, in daylight ticks");
    time_set_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    time_set_binding.connect<&cmd_time_set>();
}
