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
#ifndef CLIENT_GLOBALS_HH
#define CLIENT_GLOBALS_HH
#include <shared/globals.hh>

struct GLFWwindow;

namespace globals
{
extern GLFWwindow *window;

extern int width;
extern int height;
extern double aspect;

extern uint64_t curtime;
extern uint64_t framecount;
extern double frametime;
} // namespace globals

#endif /* CLIENT_GLOBALS_HH */
