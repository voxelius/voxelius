// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/screen.hh>
#include <client/view.hh>
#include <shared/const.hh>
#include <shared/head.hh>
#include <shared/transform.hh>
#include <shared/world.hh>

static vector3f_t view_angles = {};
static vector3f_t view_position = {};
static matrix4x4f_t view_matrix = {};

void view::update()
{
    view_angles = vector3f_t{0.0f, 0.0f, 0.0f};
    view_position = vector3f_t{0.0f, 0.0f, 0.0f};
    view_matrix = glm::perspective(cxmath::radians(90.0f), screen::get_aspect(), 0.01f, 1024.0f);

    if(globals::world.registry.valid(globals::player)) {
        const auto &head = globals::world.registry.get<HeadComponent>(globals::player);
        const auto &transform = globals::world.registry.get<TransformComponent>(globals::player);
        view_angles = vector3f_t{head.angles.x, head.angles.y, 0.0f};
        view_position = transform.position;
        view_matrix *= glm::lookAt(view_position, view_position + quatf_t{view_angles} * DIR_FORWARD, DIR_UP);
    }
}

const vector3f_t &view::get_angles()
{
    return view_angles;
}

const vector3f_t &view::get_position()
{
    return view_position;
}

const glm::mat4x4 &view::get_matrix()
{
    return view_matrix;
}
