// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_OPTIONS_HH
#define CLIENT_OPTIONS_HH
#include <shared/vfs.hh>

namespace options::general
{
extern std::string username;
} // namespace options::general

namespace options::controls
{
extern bool mouse_rawinput;
extern double mouse_sensitivity;
} // namespace options::controls

namespace options::graphics
{
extern double camera_fov;
extern unsigned int view_distance;
} // namespace options::graphics

namespace options
{
void load();
void save();
} // namespace options

#endif /* CLIENT_OPTIONS_HH */
