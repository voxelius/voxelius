// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <client/camera.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <shared/config/config.hh>
#include <shared/config/number.hh>
#include <shared/entity/head.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>

config::Number<double> camera::fov = config::Number<double>{75.0};
config::Number<unsigned int> camera::view_distance = config::Number<unsigned int>{16U};

static glm::dvec3 cam_position = {};
static glm::dvec3 cam_euler_angles = {};
static glm::dvec3 cam_direction = {};
static glm::fvec3 cam_chunk_local = {};
static chunk_pos_t cam_chunk_pos = {};
static glm::fmat4x4 cam_matrix = {};

void camera::init()
{
    config::add("camera.fov", camera::fov);
    config::add("camera.view_distance", camera::view_distance);
}

void camera::update()
{
    const double fov = cxmath::radians(camera::fov.value);
    const double zfar = camera::view_distance.value * CHUNK_SIZE;

    cam_position = glm::dvec3{};
    cam_euler_angles = glm::dvec3{};
    cam_direction = glm::dvec3{};
    cam_chunk_local = glm::fvec3{};
    cam_chunk_pos = chunk_pos_t{};
    cam_matrix = glm::perspective(fov, globals::aspect, 0.01, zfar);

    if(globals::registry.valid(globals::player)) {
        const auto &head = globals::registry.get<HeadComponent>(globals::player);
        const auto &transform = globals::registry.get<TransformComponent>(globals::player);

        cam_position = transform.position + head.offset;
        cam_euler_angles = transform.angles + glm::dvec3{head.angles.x, head.angles.y, 0.0};
        cam_direction = glm::dquat{cam_euler_angles} * DIR_FORWARD;
        cam_chunk_pos = coord::to_chunk(coord::to_voxel(transform.position));
        cam_chunk_local = cam_position - coord::to_world(cam_chunk_pos);

        cam_matrix *= glm::lookAt(cam_chunk_local, cam_chunk_local + glm::fvec3{cam_direction}, FDIR_UP);
    }
}

const glm::dvec3 &camera::get_position()
{
    return cam_position;
}

const glm::dvec3 &camera::get_euler_anlgles()
{
    return cam_euler_angles;
}

const glm::dvec3 &camera::get_direction()
{
    return cam_direction;
}

const glm::fvec3 &camera::get_chunk_local()
{
    return cam_chunk_local;
}

const chunk_pos_t &camera::get_chunk_pos()
{
    return cam_chunk_pos;
}

const glm::fmat4x4 &camera::get_matrix()
{
    return cam_matrix;
}
