// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/globals.hh>
#include <shared/config.hh>
#include <shared/entity/head.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>

config::Number camera::fov = config::Number{75.0};
config::Number camera::view_distance = config::Number{16};

static vector3d_t cam_position = {};
static vector3d_t cam_euler_angles = {};
static vector3d_t cam_direction = {};
static vector3f_t cam_chunk_local = {};
static chunk_pos_t cam_chunk_pos = {};
static matrix4x4f_t cam_matrix = {};

void camera::init()
{
    config::add_variable("camera.fov", camera::fov);
    config::add_variable("camera.view_distance", camera::view_distance);
}

void camera::update()
{
    const double fov = cxmath::radians(camera::fov.get_value());
    const double zfar = cxmath::floor<int>(camera::view_distance.get_value()) * CHUNK_SIZE;

    cam_position = vector3d_t{};
    cam_euler_angles = vector3d_t{};
    cam_direction = vector3d_t{};
    cam_chunk_local = vector3f_t{};
    cam_chunk_pos = chunk_pos_t{};
    cam_matrix = glm::perspective(fov, globals::window_aspect, 0.01, zfar);

    if(globals::registry.valid(globals::player)) {
        const auto &head = globals::registry.get<HeadComponent>(globals::player);
        const auto &transform = globals::registry.get<TransformComponent>(globals::player);

        cam_position = transform.position;
        cam_euler_angles = transform.angles + vector3d_t{head.angles.x, head.angles.y, 0.0};
        cam_direction = quaterniond_t{cam_euler_angles} * DIR_FORWARD;
        cam_chunk_pos = coord::to_chunk(coord::to_voxel(transform.position));
        cam_chunk_local = cam_position - coord::to_world(cam_chunk_pos);

        cam_matrix *= glm::lookAt(cam_chunk_local, cam_chunk_local + vector3f_t{cam_direction}, FDIR_UP);
    }
}

const vector3d_t &camera::get_position()
{
    return cam_position;
}

const vector3d_t &camera::get_euler_anlgles()
{
    return cam_euler_angles;
}

const vector3d_t &camera::get_direction()
{
    return cam_direction;
}

const vector3f_t &camera::get_chunk_local()
{
    return cam_chunk_local;
}

const chunk_pos_t &camera::get_chunk_pos()
{
    return cam_chunk_pos;
}

const matrix4x4f_t &camera::get_matrix()
{
    return cam_matrix;
}
