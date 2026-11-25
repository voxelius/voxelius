#ifndef D7C41A2E_5B90_4F63_A1D2_8C3E6F0B5A17
#define D7C41A2E_5B90_4F63_A1D2_8C3E6F0B5A17

union SDL_Event;

namespace boot
{
void start(void);
void init(void);
void shutdown(void);
} // namespace boot

namespace boot
{
void update(void);
void render(void);
void render_fade(void);
} // namespace boot

namespace boot
{
void add(std::string_view name, std::string_view key, float weight, std::function<bool(void)> step);
void set_progress(float progress);
} // namespace boot

namespace boot
{
void request_quit(void);
bool quit_requested(void);
} // namespace boot

namespace boot
{
bool active(void);
bool input(const SDL_Event& event);
} // namespace boot

#endif /* D7C41A2E_5B90_4F63_A1D2_8C3E6F0B5A17 */
