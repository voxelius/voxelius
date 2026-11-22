#include "server/pch.hh"

#include "server/world/time_sim.hh"

#include "core/config/map.hh"
#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/msg/log.hh"

#include "shared/constant.hh"
#include "shared/cvar/server.hh"
#include "shared/world/world.hh"
#include "shared/world/world_clock.hh"

#include "server/net/sessions.hh"
#include "server/universe.hh"
#include "server/world/world.hh"

constexpr static std::string_view DAYLIGHT_TICK_KEY = "daylight_tick";

static std::shared_ptr<ServerWorld> resolve_world(const cvar::Context& ctx)
{
    if(ctx.client_id) {
        auto session = sessions::find(*ctx.client_id);

        if(session && session->world) {
            return std::static_pointer_cast<ServerWorld>(session->world);
        }
    }

    return universe::default_world();
}

static void load_world(ServerWorld* world)
{
    auto& config = world->config();
    auto stored = config.value<std::uint64_t>(DAYLIGHT_TICK_KEY);

    world->daylight_tick = stored.value_or(constant::DAY_LENGTH_TICKS / 4);
}

static void save_world(ServerWorld* world)
{
    auto& config = world->config();

    config.set_value<std::uint64_t>(DAYLIGHT_TICK_KEY, world->daylight_tick);
}

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

    auto world = resolve_world(ctx);

    if(world == nullptr) {
        msg::log_error("time_set: no world to set");

        return cvar::status::UNKNOWN;
    }

    world->daylight_tick = tick;

    auto day_length = server_vars::day_length_ticks();
    auto phase = world_clock::phase(tick, day_length);
    auto day = world_clock::day(tick, day_length);
    auto id_text = world->id().full_string();

    msg::log_info("time_set: world={} daylight_tick={} day={} phase={:.3f}", id_text, tick, day, phase);

    return cvar::status::OK;
}

void time_sim::init(void)
{
    auto& time_set_binding = cvar::bind_cmd("time_set");
    time_set_binding.set_help("Set the world clock, in daylight ticks");
    time_set_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    time_set_binding.connect<&cmd_time_set>();
}

void time_sim::init_late(void)
{
    for(const auto& world : universe::worlds()) {
        load_world(world.get());
    }
}

void time_sim::shutdown(void)
{
    for(const auto& world : universe::worlds()) {
        save_world(world.get());
    }
}
