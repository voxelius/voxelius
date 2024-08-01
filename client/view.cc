// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/config.hh>

float view::vertical_fov = 90.0f;
unsigned int view::max_distance = 16U;

Vec3angles view::angles = {};
Vec3f view::direction = {};
Mat4x4f view::matrix = {};
WorldCoord view::position = {};

static void reset(void)
{
    view::angles = Vec3angles();
    view::direction = Vec3f::dir_forward();
    view::matrix = Mat4x4f::identity();
    view::position = WorldCoord();
}

void view::init(void)
{
    Config::add(globals::client_config, "view.vertical_fov", view::vertical_fov);
    Config::add(globals::client_config, "view.max_distance", view::max_distance);

    reset();
}

void view::update(void)
{
    if(!globals::registry.valid(globals::player)) {
        reset();
        return;
    }

    const auto &head = globals::registry.get<HeadComponent>(globals::player);
    const auto &tform = globals::registry.get<TransformComponent>(globals::player);

    view::angles = tform.angles + head.angles;
    view::position = tform.position;
    view::position.local += head.offset;

    // Figure out where the camera is pointed
    Vec3angles::vectors(view::angles, &view::direction, nullptr, nullptr);

    const auto z_near = 0.01f;
    const auto z_far = view::max_distance * static_cast<float>(CHUNK_SIZE);
    Mat4x4f proj = Mat4x4f::proj_persp(cxpr::radians(view::vertical_fov), globals::aspect, z_near, z_far);
    Mat4x4f view = Mat4x4f::view_psrc(view::position.local, view::angles);

    view::matrix = Mat4x4f::product(proj, view);
}
