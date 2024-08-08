// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/player_move.hh>
#include <client/screen.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/math/constexpr.hh>
#include <spdlog/spdlog.h>

constexpr static float PLAYER_MOVE_SPEED = 16.0f;
constexpr static float PLAYER_ACCELERATE = 0.05f;
constexpr static float PLAYER_DECELERATE = 0.15f;

static Vec3f wishdir = {};

void player_move::init(void)
{
    wishdir = Vec3f(0.0f, 0.0f, 0.0f);
}

void player_move::update(void)
{
    if(!globals::registry.valid(globals::player)) {
        // There's no point in updating movement
        // while we're not loaded into a world
        wishdir = Vec3f(0.0f, 0.0f, 0.0f);
        return;
    }

    if(globals::gui_screen) {
        // UI is active - player movement should not
        // update; we at least want to decelerate
        wishdir = Vec3f(0.0f, 0.0f, 0.0f);
    }

    const auto &head = globals::registry.get<HeadComponent>(globals::player);
    auto &transform = globals::registry.get<TransformComponent>(globals::player);
    auto &velocity = globals::registry.get<VelocityComponent>(globals::player);

    if(wishdir[0] || wishdir[2]) {
        Vec3f forward, right;
        Vec3angles::vectors(Vec3angles(0.0f, head.angles[1], 0.0f), &forward, &right, nullptr);
        velocity.linear[0] = cxpr::lerp(velocity.linear[0], Vec3f::dot(right, wishdir) * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
        velocity.linear[2] = cxpr::lerp(velocity.linear[2], Vec3f::dot(forward, wishdir) * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
    }
    else {
        velocity.linear[0] = cxpr::lerp(velocity.linear[0], 0.0f, PLAYER_DECELERATE);
        velocity.linear[2] = cxpr::lerp(velocity.linear[2], 0.0f, PLAYER_DECELERATE);
    }

    if(wishdir[1])
        velocity.linear[1] = cxpr::lerp(velocity.linear[1], wishdir[1] * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
    else velocity.linear[1] = cxpr::lerp(velocity.linear[1], 0.0f, PLAYER_DECELERATE);
}

void player_move::set_direction(const Vec3f &direction)
{
    wishdir = direction;
}
