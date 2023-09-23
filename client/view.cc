// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/screen.hh>
#include <client/view.hh>
#include <shared/head.hh>
#include <shared/transform.hh>

static glm::vec3 view_angles = {};
static glm::vec3 view_position = {};
static glm::mat4x4 view_matrix = {};

void view::update()
{
    view_angles = glm::vec3{0.0f, 0.0f, 0.0f};
    view_position = glm::vec3{0.0f, 0.0f, 0.0f};
    view_matrix = glm::perspective(cxmath::radians(90.0f), screen::get_aspect(), 0.01f, 1024.0f);

    if(globals::world.registry.valid(globals::player)) {
        const auto &hc = globals::world.registry.get<HeadComponent>(globals::player);
        const auto &tc = globals::world.registry.get<TransformComponent>(globals::player);
        const glm::quat qa = tc.rotation * glm::quat{hc.euler};
        view_angles = glm::eulerAngles(qa);
        view_position = tc.position;
        view_matrix *= glm::lookAt(tc.position, tc.position + qa * DIR_FORWARD, DIR_UP);
    }
}

const glm::vec3 &view::get_angles()
{
    return view_angles;
}

const glm::vec3 &view::get_position()
{
    return view_position;
}

const glm::mat4x4 &view::get_matrix()
{
    return view_matrix;
}
