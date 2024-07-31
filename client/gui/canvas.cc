// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/bitmap_font.hh>
#include <client/gui/canvas.hh>
#include <client/gui/text_vbo.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>

static GLuint program_rect_c = {};
static GLint u_rect_c_color_0 = {};
static GLint u_rect_c_color_1 = {};
static GLint u_rect_c_color_2 = {};
static GLint u_rect_c_color_3 = {};
static GLint u_rect_c_screen = {};
static GLint u_rect_c_rect = {};

static GLuint program_rect_t = {};
static GLint u_rect_t_color_0 = {};
static GLint u_rect_t_color_1 = {};
static GLint u_rect_t_color_2 = {};
static GLint u_rect_t_color_3 = {};
static GLint u_rect_t_screen = {};
static GLint u_rect_t_rect = {};

static GLuint program_text = {};
static GLint u_text_




