// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/camera.hh>
#include <client/globals.hh>
#include <client/settings.hh>
#include <entt/entity/registry.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/config.hh>
#include <shared/const.hh>

static float vertical_fov = 90.0f;
unsigned int camera::view_distance = 16U;

static EntityPos cam_position = {};
static glm::fvec3 cam_euler_angles = {};
static glm::fvec3 cam_direction = {};
static glm::fmat4x4 cam_matrix = {};

void camera::init(void)
{
    Config::add(globals::client_config, "camera.vertical_fov", vertical_fov);
    Config::add(globals::client_config, "camera.view_distance", camera::view_distance);

    settings::add_slider(1, settings::GENERAL, "camera.vertical_fov", vertical_fov, 54.0f, 110.0f, true, "%.0f");
    settings::add_slider(0, settings::VIDEO, "camera.view_distance", camera::view_distance, 4U, 32U, false);

    cam_position = EntityPos();
    cam_euler_angles = glm::fvec3();
    cam_direction = DIR_FORWARD;
    cam_matrix = glm::identity<glm::fmat4x4>();
}

void camera::update(void)
{
    if(!globals::registry.valid(globals::player)) {
        cam_position = EntityPos();
        cam_euler_angles = glm::fvec3();
        cam_direction = DIR_FORWARD;
        cam_matrix = glm::identity<glm::fmat4x4>();
        return;
    }

    const auto &head = globals::registry.get<HeadComponent>(globals::player);
    const auto &transform = globals::registry.get<TransformComponent>(globals::player);

    cam_position = transform.position;
    cam_position.local + head.offset;
    cam_euler_angles = glm::fvec3(head.angles.x, head.angles.y, 0.0f);
    cam_direction = glm::fquat(cam_euler_angles) * DIR_FORWARD;

    const float fov = glm::radians(vertical_fov);
    const float zfar = camera::view_distance * CHUNK_SIZE;

    const glm::fmat4x4 proj = glm::perspective(fov, globals::aspect, 0.01f, zfar);
    const glm::fmat4x4 view = glm::lookAt(cam_position.local, cam_position.local + cam_direction, DIR_UP);
    cam_matrix = proj * view;
}

const EntityPos &camera::position(void)
{
    return cam_position;
}

const glm::fvec3 &camera::euler_angles(void)
{
    return cam_euler_angles;
}

const glm::fvec3 &camera::direction(void)
{
    return cam_direction;
}

const glm::fmat4x4 &camera::matrix(void)
{
    return cam_matrix;
}
