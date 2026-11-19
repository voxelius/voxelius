#ifndef B65ECA83_B7F0_4ED9_A3E3_195FE13ABE37
#define B65ECA83_B7F0_4ED9_A3E3_195FE13ABE37

namespace gui
{
class Container;
class Screen;
} // namespace gui

namespace settings
{
extern gui::Screen screen;
} // namespace settings

namespace settings
{
extern gui::Container general;
extern gui::Container controls_movement;
extern gui::Container controls_gameplay;
extern gui::Container controls_miscellaneous;
extern gui::Container mouse;
extern gui::Container video;
extern gui::Container video_lod;
extern gui::Container video_atmosphere;
extern gui::Container video_gui;
extern gui::Container sound;
extern gui::Container sound_levels;
} // namespace settings

namespace settings::detail
{
std::optional<bool> video_mode_popup(const std::optional<SDL_DisplayMode>& mode);
} // namespace settings::detail

namespace settings
{
void init(void);
void shutdown(void);
} // namespace settings

#endif /* B65ECA83_B7F0_4ED9_A3E3_195FE13ABE37 */
