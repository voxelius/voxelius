// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/options.hh>
#include <shared/properties.hh>
#include <spdlog/spdlog.h>

constexpr static const char *CONFIG_PATH = "options.conf";

std::string options::general::username = "player";

bool options::controls::mouse_rawinput = true;
double options::controls::mouse_sensitivity = 0.25;

double options::graphics::camera_fov = 90.0;
unsigned int options::graphics::view_distance = 8U;

void options::load()
{
    Properties properties = {};
    if(properties.read(CONFIG_PATH)) {
        options::general::username = properties.get_string("general", "username", "player");
        options::controls::mouse_rawinput = properties.get_bool("controls", "mouse_rawinput", true);
        options::controls::mouse_sensitivity = properties.get_double("controls", "mouse_sensitivity", 0.25);
        options::graphics::camera_fov = properties.get_int("graphics", "camera_fov", 90);
        options::graphics::view_distance = properties.get_int("graphics", "view_distance", 8);
    }
}

void options::save()
{
    Properties properties = {};
    properties.set_string("general", "username", options::general::username);
    properties.set_bool("controls", "mouse_rawinput", options::controls::mouse_rawinput);
    properties.set_double("controls", "mouse_sensitivity", options::controls::mouse_sensitivity);
    properties.set_int("graphics", "camera_fov", options::graphics::camera_fov);
    properties.set_int("graphics", "view_distance", options::graphics::view_distance);
    properties.write(CONFIG_PATH);
}

