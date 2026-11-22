#ifndef B37CFBC1_EB9E_4C2A_9DCE_EF5C37C1E41F
#define B37CFBC1_EB9E_4C2A_9DCE_EF5C37C1E41F

namespace gui
{
class Screen;
} // namespace gui

namespace play_menu
{
extern gui::Screen screen;
} // namespace play_menu

namespace play_menu
{
void init(void);
void shutdown(void);
void update_late(void);
} // namespace play_menu

namespace play_menu
{
void show_error(std::string_view message);
} // namespace play_menu

#endif /* B37CFBC1_EB9E_4C2A_9DCE_EF5C37C1E41F */
