#include "client/pch.hh"

#include "client/globals.hh"

World* globals::world;

SDL_Window* globals::window;
SDL_GLContext globals::gl_context;

int globals::width;
int globals::height;
float globals::aspect;

std::uint64_t globals::fixed_accumulator_us;
std::size_t globals::window_framecount;
std::uint64_t globals::window_frametime_us;
float globals::window_frametime;
float globals::window_frametime_avg;

std::size_t globals::num_draw_calls;
std::size_t globals::num_draw_vertices;

entt::entity globals::player;

unsigned globals::gui_scale = 1U;
gui::Screen* globals::gui_screen = nullptr;

ENetHost* globals::host;
ENetPeer* globals::peer;
