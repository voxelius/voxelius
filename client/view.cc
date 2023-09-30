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

static vector3_t view_angles {};
static vector3_t view_position {};
static vector3_t view_direction {};
static vector3f_t view_lpos_float {};
static matrix4x4f_t view_matrix {};
static chunk_pos_t view_cpos {};

void view::update()
{
    view_angles = vector3_t{};
    view_position = vector3_t{};
    view_direction = vector3_t{};
    view_lpos_float = vector3f_t{};
    view_matrix = glm::perspective(cxmath::radians(90.0), screen::get_aspect(), 0.01, 1024.0);
    view_cpos = chunk_pos_t{};

    if(globals::world.registry.valid(globals::player)) {
        const auto &head = globals::world.registry.get<HeadComponent>(globals::player);
        const auto &transform = globals::world.registry.get<TransformComponent>(globals::player);
        view_cpos = coord::to_chunk(coord::to_voxel(transform.position));
        view_angles = transform.angles + vector3_t{head.angles.x, head.angles.y, 0.0};
        view_position = transform.position; // FIXME: head offset?
        view_direction = quaternion_t{view_angles} * DIR_FORWARD;
        view_lpos_float = view_position - coord::to_world(view_cpos);
        view_matrix *= glm::lookAt(view_lpos_float, view_lpos_float + vector3f_t{view_direction}, FDIR_UP);
    }
}

const vector3_t &view::get_angles()
{
    return view_angles;
}

const vector3_t &view::get_position()
{
    return view_position;
}

const vector3_t &view::get_direction()
{
    return view_direction;
}

const vector3f_t &view::get_lpos_float()
{
    return view_lpos_float;
}

const matrix4x4f_t &view::get_matrix()
{
    return view_matrix;
}

const chunk_pos_t &view::get_cpos()
{
    return view_cpos;
}

void view::get_values(ViewValues &vv)
{
    vv.angles = view_angles;
    vv.position = view_position;
    vv.direction = view_direction;
    vv.lpos_float = view_lpos_float;
    vv.matrix = view_matrix;
    vv.cpos = view_cpos;
}
