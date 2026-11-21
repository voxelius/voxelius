#include "server/pch.hh"

#include "server/game.hh"

#include <charconv>
#include <system_error>

#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/identifier.hh"
#include "core/msg/log.hh"

#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/constant.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/entity.hh"
#include "shared/world/lighting.hh"
#include "shared/world/world.hh"

#include "server/net/sessions.hh"
#include "server/system/pmove_validator.hh"
#include "server/universe.hh"
#include "server/world/time_sim.hh"
#include "server/world/weather_sim.hh"
#include "server/world/world.hh"

static cvar::status cmd_stop(const cvar::Context&, std::span<const std::string_view>)
{
    std::raise(SIGINT);

    return cvar::status::OK;
}

static bool parse_entity(std::string_view text, entt::entity& entity)
{
    std::uint32_t value = 0;
    auto first = text.data();
    auto last = first + text.size();
    auto result = std::from_chars(first, last, value);

    if(result.ec != std::errc() || result.ptr != last) {
        return false;
    }

    entity = static_cast<entt::entity>(value);
    return true;
}

static bool parse_position(std::span<const std::string_view> args, BlockPos& position)
{
    BlockPos::value_type values[3] {};

    for(std::size_t i = 0; i < 3; ++i) {
        auto text = args[2 + i];
        auto first = text.data();
        auto last = first + text.size();
        auto result = std::from_chars(first, last, values[i]);

        if(result.ec != std::errc() || result.ptr != last) {
            return false;
        }
    }

    position = BlockPos(values[0], values[1], values[2]);
    return true;
}

static cvar::status cmd_tpe(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.size() != 2 && args.size() != 5) {
        msg::log_error("usage: tpe <entity> <world> [X Y Z]");
        return cvar::status::ARGUMENT;
    }

    entt::entity entity = entt::null;

    if(!parse_entity(args[0], entity)) {
        msg::log_error("tpe: invalid entity id: {}", args[0]);
        return cvar::status::ARGUMENT;
    }

    auto world_id = Identifier::from_string(args[1], constant::BUILTIN_NAME_SPACE);
    auto world = universe::find(world_id);

    if(world == nullptr) {
        msg::log_error("tpe: unknown world: {}", args[1]);
        return cvar::status::ARGUMENT;
    }

    std::optional<BlockPos> position;

    if(5 == args.size()) {
        BlockPos parsed {};

        if(!parse_position(args, parsed)) {
            msg::log_error("tpe: invalid position");
            return cvar::status::ARGUMENT;
        }

        position = parsed;
    }

    auto source = std::shared_ptr<ServerWorld>();

    if(ctx.client_id) {
        auto session = sessions::find(*ctx.client_id);

        if(session && session->world) {
            auto& registry = session->world->registry();

            if(registry.valid(entity)) {
                source = std::static_pointer_cast<ServerWorld>(session->world);
            }
        }
    }

    if(source == nullptr) {
        for(const auto& candidate : universe::worlds()) {
            auto& registry = candidate->registry();

            if(registry.valid(entity)) {
                source = candidate;
                break;
            }
        }
    }

    if(source == nullptr) {
        msg::log_error("tpe: entity {} does not exist", args[0]);
        return cvar::status::ARGUMENT;
    }

    auto& registry = source->registry();
    auto& definition = world->definition();
    auto destination = definition.spawn;

    if(auto transform = registry.try_get<Transform>(entity)) {
        destination = utils::to_block(transform->chunk, transform->local.cast<LocalPos::value_type>());
    }

    if(position.has_value()) {
        destination = position.value();
    }

    if(registry.all_of<SessionRef>(entity)) {
        auto& ref = registry.get<SessionRef>(entity);
        auto session = ref.ptr;

        if(!sessions::transfer(session, world, position)) {
            msg::log_error("tpe: failed to transfer the session into {}", args[1]);
            return cvar::status::UNKNOWN;
        }

        return cvar::status::OK;
    }

    if(!registry.all_of<Transform>(entity)) {
        msg::log_warning("tpe: entity {} has no transform; the position is ignored", args[0]);
    }

    auto& target_registry = world->registry();
    auto result = utils::transfer(registry, entity, target_registry);

    if(result == entt::null) {
        msg::log_error("tpe: failed to transfer entity {}", args[0]);
        return cvar::status::UNKNOWN;
    }

    if(target_registry.all_of<Transform>(result)) {
        target_registry.patch<Transform>(result, [&](Transform& transform) {
            transform.chunk = utils::to_chunk(destination);

            auto local = utils::to_local(destination);
            transform.local = local.cast<float>();
        });
    }

    return cvar::status::OK;
}

void server_game::init(void)
{
    lighting::set_replica(false);

    gamerules::init();
    server_vars::init();

    pmove_validator::init();
    time_sim::init();

    auto& stop_binding = cvar::bind_cmd("stop");
    stop_binding.set_help("Stop the server");
    stop_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    stop_binding.connect<&cmd_stop>();

    auto& tpe_binding = cvar::bind_cmd("tpe");
    tpe_binding.set_help("Teleport an entity or a player to another world");
    tpe_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    tpe_binding.connect<&cmd_tpe>();
}

void server_game::init_late(void)
{
    auto& default_world = universe::default_world();
    auto& registry = default_world->registry();

    utils::spawn(registry, Identifier::from_string("builtin:test_entity"));

    // Mods and the universe seed are both ready by this point
    weather_sim::init();
}

void server_game::shutdown(void)
{
    weather_sim::shutdown();
}

void server_game::fixed_update(World* world)
{
    ZoneScoped;

    if(world) {
        auto& registry = world->registry();

        Transform::fixed_update(registry);
        Velocity::fixed_update(registry);

        world->fixed_update();

        pmove::update(registry, globals::fixed_frametime);
    }
}

void server_game::fixed_update_late(void)
{
    ZoneScoped;

    pmove_validator::fixed_update_late();
    weather_sim::fixed_update();
}
