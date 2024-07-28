// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/settings.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/config.hh>
#include <shared/const.hh>

float view::vertical_fov = 90.0f;
unsigned int view::max_distance = 16U;

Angle3D view::angles = {};
Vector3D view::direction = {};
Matrix4x4 view::matrix = {};
WorldPos view::position = {};

static void reset(void)
{
    view::angles = Angle3D();
    view::direction = DIR_FORWARD;
    view::matrix = Matrix4x4::identity();
    view::position = WorldPos();
}

void view::init(void)
{
    Config::add(globals::client_config, "view.vertical_fov", view::vertical_fov);
    Config::add(globals::client_config, "view.max_distance", view::max_distance);

    settings::add_slider(1, settings::GENERAL, "view.vertical_fov", view::vertical_fov, 45.0, 120.0f, true, "%.0f");
    settings::add_slider(0, settings::VIDEO, "view.max_distance", view::max_distance, 4U, 64U, false);

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
    Angle3D::vectors(view::angles, &view::direction, nullptr, nullptr);

    const auto z_near = 0.01f;
    const auto z_far = view::max_distance * static_cast<float>(CHUNK_SIZE);
    Matrix4x4 proj = Matrix4x4::persp(cxpr::radians(view::vertical_fov), globals::aspect, z_near, z_far);
    Matrix4x4 view = Matrix4x4::psrc_view(view::position.local, view::angles);

    view::matrix = Matrix4x4::product(proj, view);
}
