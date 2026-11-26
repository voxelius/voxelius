#include "shared/pch.hh"

#include "shared/system/pmove.hh"

#include "shared/component/collision.hh"
#include "shared/component/grounded.hh"
#include "shared/component/move_mode.hh"
#include "shared/component/player.hh"
#include "shared/component/stasis.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/physics/collision.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/world.hh"

constexpr static float PMOVE_EPS = 1.0e-6f;
constexpr static float FLY_SPEED = 16.0f;
constexpr static float FLY_ACCEL_H = 64.0f;
constexpr static float FLY_ACCEL_V = 96.0f;

constexpr static float WALK_JUMP_VELOCITY = 0.42f;
constexpr static float WALK_GRAVITY = 0.08f;
constexpr static float WALK_DRAG_H = 0.91f;
constexpr static float WALK_DRAG_V = 0.98f;
constexpr static float WALK_GROUND_FRICTION = 0.6f;
constexpr static float WALK_INPUT_GROUND = 0.1f;
constexpr static float WALK_INPUT_AIR = 0.02f;
constexpr static float WALK_BOUNCE_MIN = 0.1f;

// Fixed ticks between two jumps, the same delay Minecraft uses
constexpr static std::uint32_t WALK_JUMP_COOLDOWN = 10;

constexpr static float PUSH_STRENGTH = 0.05f;
constexpr static float PUSH_MAX = 0.1f;

static bool s_walk_warning_shown = false;

static Eigen::Vector3f accelerate(const Eigen::Vector3f& target, const Eigen::Vector3f& velocity, float accel, float frametime)
{
    auto delta = target - velocity;
    auto distance = delta.norm();

    if(distance < accel * frametime || distance < PMOVE_EPS) {
        return target;
    }

    return velocity + delta * (accel * frametime / distance);
}

static float accelerate(float target, float velocity, float accel, float frametime)
{
    auto delta = target - velocity;
    auto distance = std::fabs(delta);

    if(distance < accel * frametime || distance < PMOVE_EPS) {
        return target;
    }

    return velocity + delta * (accel * frametime / distance);
}

static float friction_speed_scale(float friction)
{
    auto ratio = WALK_GROUND_FRICTION / friction;
    return ratio * ratio * ratio;
}

static void update_fly(entt::registry& registry, entt::entity entity, const MoveData& input, float frametime)
{
    auto wishdir = input.wishdir;

    if(!wishdir.isZero()) {
        wishdir.normalize();
    }

    registry.patch<Velocity>(entity, [&](Velocity& velocity) {
        auto target_h = Eigen::Vector3f(wishdir.x() * FLY_SPEED, 0.0f, wishdir.z() * FLY_SPEED);
        auto current_h = Eigen::Vector3f(velocity.value.x(), 0.0f, velocity.value.z());
        auto new_h = accelerate(target_h, current_h, FLY_ACCEL_H, frametime);

        auto new_y = accelerate(wishdir.y() * FLY_SPEED, velocity.value.y(), FLY_ACCEL_V, frametime);

        velocity.value = Eigen::Vector3f(new_h.x(), new_y, new_h.z());
    });
}

static void integrate_fly(entt::registry& registry, entt::entity entity, float frametime)
{
    auto& transform = registry.get<Transform>(entity);
    auto& velocity = registry.get<Velocity>(entity);

    auto speed_sq = velocity.value.squaredNorm();

    if(speed_sq >= constant::VELOCITY_EPS_SQ) {
        transform.local += velocity.value * frametime;
    }
    else if(speed_sq) {
        velocity.value.setZero();
    }

    transform.normalize();

    registry.patch<Transform>(entity);
    registry.patch<Velocity>(entity);
}

static Eigen::Vector3f collect_push(entt::registry& registry, entt::entity entity, const Transform& transform, const Collision& collision)
{
    auto self_box = physics::bounds(Eigen::Vector3f::Zero(), collision.local_box);
    Eigen::Vector3f push = Eigen::Vector3f::Zero();

    auto view = registry.view<Collision, Transform>(entt::exclude<Stasis>);

    for(auto [other, other_collision, other_transform] : view.each()) {
        if(other == entity) {
            continue;
        }

        auto delta = utils::to_relative(transform.chunk, transform.local, other_transform.chunk, other_transform.local);
        auto other_box = physics::bounds(delta, other_collision.local_box);

        if(!self_box.intersects(other_box)) {
            continue;
        }

        auto distance_sq = delta.x() * delta.x() + delta.z() * delta.z();

        if(distance_sq < 1.0e-4f) {
            continue;
        }

        auto distance = std::sqrt(distance_sq);

        push.x() -= (delta.x() / distance) * PUSH_STRENGTH;
        push.z() -= (delta.z() / distance) * PUSH_STRENGTH;
    }

    auto length = push.norm();

    if(length > PUSH_MAX) {
        push *= PUSH_MAX / length;
    }

    return push;
}

static void apply_bounce(const physics::MoveResult& result, const Eigen::Vector3f& before, Eigen::Vector3f& movement)
{
    for(std::size_t axis = 0; axis < 3; axis += 1) {
        const auto& contact = result.axis_contacts[axis];

        if(contact.block == BLOCK_ID_NULL) {
            continue;
        }

        auto definition = block_registry::find_definition(contact.block);

        if(definition == nullptr) {
            continue;
        }

        auto restitution = definition->surface.bounce[axis];

        if(restitution == 0.0f || std::abs(before[axis]) <= WALK_BOUNCE_MIN) {
            continue;
        }

        movement[axis] = -before[axis] * restitution;
    }
}

static void update_walk(entt::registry& registry, entt::entity entity, World* world, const MoveData& input, Transform& transform,
    Velocity& velocity, const Collision& collision, float frametime)
{
    auto grounded = registry.try_get<GroundedFlag>(entity);
    bool was_on_ground = grounded;
    auto jump_requested = input.wishdir.y() > 0.0f;

    const BlockDefinition* support = nullptr;

    if(grounded) {
        support = block_registry::find_definition(grounded->surface);
    }

    auto fluid = physics::sample_fluid(world, transform.chunk, transform.local, collision.local_box);
    auto fluid_def = fluid_registry::find_definition(fluid.fluid);
    bool in_fluid = fluid_def && fluid.submersion > 0.0f;

    std::uint32_t cooldown_ticks = 0;

    if(!in_fluid) {
        if(auto cooldown = registry.try_get<JumpCooldown>(entity)) {
            cooldown_ticks = cooldown->ticks;
        }

        if(cooldown_ticks) {
            cooldown_ticks -= 1;
        }

        if(!jump_requested) {
            cooldown_ticks = 0;
        }
    }

    Eigen::Vector3f movement = velocity.value * frametime;

    if(in_fluid) {
        if(jump_requested) {
            movement.y() += fluid_def->physics.swim_speed;
        }
    }
    else if(jump_requested && was_on_ground && cooldown_ticks == 0) {
        movement.y() = WALK_JUMP_VELOCITY;
        cooldown_ticks = WALK_JUMP_COOLDOWN;
    }

    if(auto cooldown = registry.try_get<JumpCooldown>(entity)) {
        cooldown->ticks = cooldown_ticks;
    }
    else {
        JumpCooldown next;
        next.ticks = cooldown_ticks;
        registry.emplace<JumpCooldown>(entity, next);
    }

    auto horizontal = Eigen::Vector3f(input.wishdir.x(), 0.0f, input.wishdir.z());
    auto length = horizontal.norm();

    if(length > 0.0f) {
        auto factor = WALK_INPUT_AIR;

        if(in_fluid) {
            factor = fluid_def->physics.input_scale;
        }
        else if(was_on_ground) {
            auto friction = WALK_GROUND_FRICTION;
            auto speed = 1.0f;

            if(support) {
                friction = support->surface.friction;
                speed = support->surface.speed;
            }

            factor = WALK_INPUT_GROUND * speed * friction_speed_scale(friction);
        }

        movement += horizontal * (factor / std::max(1.0f, length));
    }

    if(in_fluid) {
        movement += fluid.flow * (fluid.submersion * fluid_def->physics.flow_scale);
    }

    movement += collect_push(registry, entity, transform, collision);

    auto before = movement;
    physics::Body body;
    body.local_box = collision.local_box;
    body.step_height = collision.step_height;

    auto result = physics::move(world, transform.chunk, transform.local, movement, body, was_on_ground);

    transform.local = result.position;
    transform.normalize();

    movement = result.movement;

    apply_bounce(result, before, movement);

    if(in_fluid) {
        auto hit_horizontal = result.axis_contacts[0].block || result.axis_contacts[2].block;

        if(jump_requested && hit_horizontal) {
            movement.y() = fluid_def->physics.climb_speed;
        }

        const auto& fluid_physics = fluid_def->physics;

        movement.x() *= fluid_physics.drag;
        movement.z() *= fluid_physics.drag;
        movement.y() = movement.y() * fluid_physics.drag - WALK_GRAVITY * fluid_physics.gravity_scale;
    }
    else {
        movement.x() *= WALK_DRAG_H;
        movement.z() *= WALK_DRAG_H;
        movement.y() = movement.y() * WALK_DRAG_V - WALK_GRAVITY;

        if(result.on_ground) {
            auto friction = WALK_GROUND_FRICTION;

            if(auto definition = block_registry::find_definition(result.support.block)) {
                friction = definition->surface.friction;
            }

            movement.x() *= friction;
            movement.z() *= friction;
        }
    }

    velocity.value = movement / frametime;

    if(result.on_ground) {
        if(auto grounded = registry.try_get<GroundedFlag>(entity)) {
            grounded->surface = result.support.block;
        }
        else {
            GroundedFlag next;
            next.surface = result.support.block;
            registry.emplace<GroundedFlag>(entity, next);
        }
    }
    else if(registry.all_of<GroundedFlag>(entity)) {
        registry.remove<GroundedFlag>(entity);
    }

    registry.patch<Transform>(entity);
    registry.patch<Velocity>(entity);
}

static void update_fly_mode(entt::registry& registry, entt::entity entity, const MoveData& input, Transform& transform, Velocity& velocity)
{
    auto frametime = globals::fixed_frametime;

    update_fly(registry, entity, input, frametime);

    if(auto collision = registry.try_get<Collision>(entity)) {
        auto push = collect_push(registry, entity, transform, *collision);
        velocity.value += push / frametime;
    }

    integrate_fly(registry, entity, frametime);

    if(registry.all_of<GroundedFlag>(entity)) {
        registry.remove<GroundedFlag>(entity);
    }
}

static void update_walk_mode(entt::registry& registry, entt::entity entity, World* world, const MoveData& input, Transform& transform,
    Velocity& velocity)
{
    auto collision = registry.try_get<Collision>(entity);

    if(collision == nullptr) {
        if(!s_walk_warning_shown) {
            msg::log_warning("entity {} walks without a collision component", static_cast<std::uint64_t>(entity));
            s_walk_warning_shown = true;
        }

        return;
    }

    update_walk(registry, entity, world, input, transform, velocity, *collision, globals::fixed_frametime);
}

void pmove::fixed_update(entt::registry& registry)
{
    auto world = World::of(registry);

    if(world == nullptr) {
        return;
    }

    auto view = registry.view<MoveData, MoveMode, Player, Transform, Velocity>(entt::exclude<Stasis>);

    for(auto [entity, move_data, move_mode, transform, velocity] : view.each()) {
        switch(move_mode.value) {
            case MoveMode::FLY:
                update_fly_mode(registry, entity, move_data, transform, velocity);
                break;

            case MoveMode::WALK:
                update_walk_mode(registry, entity, world, move_data, transform, velocity);
                break;
        }
    }
}
