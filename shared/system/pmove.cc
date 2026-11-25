#include "shared/pch.hh"

#include "shared/system/pmove.hh"

#include "core/config/map.hh"
#include "core/config/ref.hh"

#include "shared/component/head.hh"
#include "shared/component/move_mode.hh"
#include "shared/component/player.hh"
#include "shared/component/stasis.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"

constexpr static float PMOVE_EPS = 1.0e-6f;
constexpr static float FLY_SPEED = 16.0f;
constexpr static float FLY_ACCEL_H = 64.0f;
constexpr static float FLY_ACCEL_V = 96.0f; // Sharper movement vertically

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

static void update_fly(entt::registry& registry, entt::entity entity, const Eigen::Vector3f& wishdir, float frametime)
{
    registry.patch<Velocity>(entity, [&](Velocity& velocity) {
        auto target_h = Eigen::Vector3f(wishdir.x() * FLY_SPEED, 0.0f, wishdir.z() * FLY_SPEED);
        auto current_h = Eigen::Vector3f(velocity.value.x(), 0.0f, velocity.value.z());
        auto new_h = accelerate(target_h, current_h, FLY_ACCEL_H, frametime);

        auto new_y = accelerate(wishdir.y() * FLY_SPEED, velocity.value.y(), FLY_ACCEL_V, frametime);

        velocity.value = Eigen::Vector3f(new_h.x(), new_y, new_h.z());
    });
}

static void update_walk(entt::entity, const Eigen::Vector3f&, float)
{
    msg::log_warning("no walk move implemented yet");
}

void pmove::update(entt::registry& registry, float frametime)
{
    auto view = registry.view<MoveData, MoveMode, Player, Velocity>(entt::exclude<Stasis>);

    for(const auto [entity, move_data, move_mode, velocity] : view.each()) {
        switch(move_mode.value) {
            case MoveMode::FLY:
                update_fly(registry, entity, move_data.wishdir, frametime);
                break;

            case MoveMode::WALK:
                update_walk(entity, move_data.wishdir, frametime);
                break;
        }
    }
}
