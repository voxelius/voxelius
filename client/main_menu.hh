#ifndef B50F923C_FC26_4E20_B259_B53791A35A9B
#define B50F923C_FC26_4E20_B259_B53791A35A9B

namespace gui
{
class Screen;
class Background;
class Dimmer;
} // namespace gui

namespace main_menu
{
extern gui::Screen screen;
extern gui::Background background;
} // namespace main_menu

namespace main_menu
{
void init(void);
void init_late(void);
void shutdown(void);
} // namespace main_menu

namespace main_menu
{
void show(void);
} // namespace main_menu

#endif /* B50F923C_FC26_4E20_B259_B53791A35A9B */
