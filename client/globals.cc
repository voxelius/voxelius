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
#include <client/globals.hh>
#include <GLFW/glfw3.h>

GLFWwindow *globals::window = nullptr;

int globals::width = 0;
int globals::height = 0;
double globals::aspect = 0.0;

uint64_t globals::curtime = 0;
uint64_t globals::framecount = 0;
double globals::frametime = 0.0;
