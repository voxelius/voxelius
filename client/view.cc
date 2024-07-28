// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/settings.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/config.hh>

float view::vertical_fov = 90.0f;
unsigned int view::max_distance = 16U;

glm::fvec3 view::angles = {};
glm::fvec3 view::direction = {};
glm::fmat4x4 view::matrix = {};
EntityPos view::position = {};

static void reset(void)
{
    view::angles = glm::fvec3();
    view::direction = DIR_FORWARD;
    view::matrix = glm::identity<glm::fmat4x4>();
    view::position = EntityPos();
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

    const auto p_cv = glm::cos(view::angles);
    const auto p_sv = glm::sin(view::angles);

    // Flipped sine and cosine values because
    // they're used pretty frequently for calculations
    const auto n_cv = -1.0f * p_cv;
    const auto n_sv = -1.0f * p_sv;

    view::direction.x = n_cv.y * p_sv.x * p_sv.z - p_sv.y * p_cv.z;
    view::direction.y = n_sv.y * p_sv.x * p_sv.z + p_cv.y * p_cv.z;
    view::direction.z = p_cv.x * p_sv.z;

    const auto znear = 0.01f;
    const auto zfar = view::max_distance * static_cast<float>(CHUNK_SIZE);
    const auto tan_half_fov = glm::tan(glm::radians(view::vertical_fov) * 0.5f);

    glm::fmat4x4 projmat = glm::fmat4x4(0.0f);
    projmat[0][0] = 1.0f / (tan_half_fov * globals::aspect);
    projmat[1][1] = 1.0f / (tan_half_fov);
    projmat[2][2] = -1.0f * (zfar + znear) / (zfar - znear);
    projmat[2][3] = -1.0f;
    projmat[3][2] = -2.0f * zfar * znear / (zfar - znear);

    glm::fvec3 up = {};
    up.x = p_sv.x * p_sv.y * p_cv.z + n_cv.y * p_sv.z;
    up.y = p_cv.x * p_cv.z;
    up.z = n_sv.x * n_cv.y * p_cv.z + p_sv.y * p_sv.z;

    view::direction.x = p_cv.x * n_sv.y;
    view::direction.y = p_sv.x;
    view::direction.z = p_cv.x * n_cv.y;

    glm::fmat4x4 viewmat = glm::fmat4x4(1.0f);
    viewmat[0][0] = view::direction.y * up.z - view::direction.z * up.y;
    viewmat[1][0] = view::direction.z * up.x - view::direction.x * up.z;
    viewmat[2][0] = view::direction.x * up.y - view::direction.y * up.x;
    viewmat[3][0] = -viewmat[0][0] * view::position.local.x - viewmat[1][0] * view::position.local.y - viewmat[2][0] * view::position.local.z;
    viewmat[0][1] = up.x;
    viewmat[1][1] = up.y;
    viewmat[2][1] = up.z;
    viewmat[3][1] = -up.x * view::position.local.x - up.y * view::position.local.y - up.z * view::position.local.z;
    viewmat[0][2] = -view::direction.x;
    viewmat[1][2] = -view::direction.y;
    viewmat[2][2] = -view::direction.z;
    viewmat[3][2] = view::direction.x * view::position.local.x + view::direction.y * view::position.local.y + view::direction.z * view::position.local.z;

    view::matrix = projmat * viewmat;
}
