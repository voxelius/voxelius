#ifndef EA5B6422_3EC6_4652_8657_A31916400E3A
#define EA5B6422_3EC6_4652_8657_A31916400E3A

namespace video
{
extern bool mode_pending;
} // namespace video

namespace video
{
void init(void);
void init_late(void);
void shutdown(void);
void update(void);
void update_late(void);
} // namespace video

namespace video
{
void query_current_mode(int& width, int& height);
void query_current_mode(int& width, int& height, bool& fullscreen);
void query_current_mode(int& width, int& height, int& rate, bool& fullscreen);
const std::vector<SDL_DisplayMode>& query_fullscreen_modes(void);
} // namespace video

namespace video
{
void request_fullscreen(int width, int height, int rate);
void request_windowed(int width, int height);
void request_windowed(void);
} // namespace video

namespace video
{
void update_window_title(void);
} // namespace video

#endif /* EA5B6422_3EC6_4652_8657_A31916400E3A */
