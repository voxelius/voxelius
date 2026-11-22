#ifndef D618E26B_01F4_4D5C_A249_6D2B49637425
#define D618E26B_01F4_4D5C_A249_6D2B49637425

#include "shared/globals.hh"

class World;

namespace gui
{
class Screen;
} // namespace gui

namespace globals
{
extern World* world;
} // namespace globals

namespace globals
{
extern SDL_Window* window;
extern SDL_GLContext gl_context;
} // namespace globals

namespace globals
{
extern int width;
extern int height;
extern float aspect;
} // namespace globals

namespace globals
{
extern std::uint64_t fixed_accumulator_us;
extern std::size_t window_framecount;
extern std::uint64_t window_frametime_us;
extern float window_frametime;
extern float window_frametime_avg;
} // namespace globals

namespace globals
{
extern std::size_t num_draw_calls;
extern std::size_t num_draw_vertices;
} // namespace globals

namespace globals
{
extern entt::entity player;
} // namespace globals

namespace globals
{
extern unsigned gui_scale;
extern gui::Screen* gui_screen;
} // namespace globals

namespace globals
{
extern ENetHost* host;
extern ENetPeer* peer;
} // namespace globals

#endif /* D618E26B_01F4_4D5C_A249_6D2B49637425 */
