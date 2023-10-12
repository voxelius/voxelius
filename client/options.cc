// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/options.hh>
#include <shared/properties.hh>

void Options::read(const vfs::path_t &path, Options &out)
{
    Properties properties = {};

    if(properties.read(path)) {
        // General
        out.general.fov = properties.get_int("general", "fov", 90);

        // Controls
        out.controls.raw_mouse = properties.get_bool("controls", "raw_mouse", true);
        out.controls.sensitivity = properties.get_double("controls", "sensitivity", 0.25);

        // Graphics
        // Nothing yet

        // Sound
        // Nothing yet
    }
}

void Options::write(const vfs::path_t &path, const Options &in)
{
    Properties properties = {};

    // General
    properties.set_int("general", "fov", in.general.fov);

    // Controls
    properties.set_bool("controls", "raw_mouse", in.controls.raw_mouse);
    properties.set_double("controls", "sensitivity", in.controls.sensitivity);

    // Graphics
    // Nothing yet

    // Sound
    // Nothing yet

    properties.write(path);
}
