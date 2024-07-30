// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/player_move.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/math/constexpr.hh>

constexpr static float PLAYER_MOVE_SPEED = 16.0f;
constexpr static float PLAYER_ACCELERATE = 0.05f;
constexpr static float PLAYER_DECELERATE = 0.15f;

static Vec3f pmove_wishdir = {};

void player_move::init(void)
{
    pmove_wishdir = Vec3f();
}

void player_move::update(void)
{
    if(!globals::registry.valid(globals::player)) {
        // There's no point in updating movement
        // while we're not loaded into a world
        return;
    }

    const auto &head = globals::registry.get<HeadComponent>(globals::player);
    auto &transform = globals::registry.get<TransformComponent>(globals::player);
    auto &velocity = globals::registry.get<VelocityComponent>(globals::player);

    if(pmove_wishdir[0] || pmove_wishdir[2]) {
        Vec3f forward, right;
        Vec3angles::vectors(Vec3angles(0.0f, head.angles[1], 0.0f), &forward, &right, nullptr);
        velocity.linear[0] = cxpr::lerp(velocity.linear[0], Vec3f::dot(right, pmove_wishdir) * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
        velocity.linear[2] = cxpr::lerp(velocity.linear[2], Vec3f::dot(forward, pmove_wishdir) * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
    }
    else {
        velocity.linear[0] = cxpr::lerp(velocity.linear[0], 0.0f, PLAYER_DECELERATE);
        velocity.linear[2] = cxpr::lerp(velocity.linear[2], 0.0f, PLAYER_DECELERATE);
    }

    if(pmove_wishdir[1])
        velocity.linear[1] = cxpr::lerp(velocity.linear[1], pmove_wishdir[1] * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
    else velocity.linear[1] = cxpr::lerp(velocity.linear[1], 0.0f, PLAYER_DECELERATE);
}

void player_move::set_direction(const Vec3f &direction)
{
    pmove_wishdir = direction;
}
