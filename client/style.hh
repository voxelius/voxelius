#ifndef F132A6BF_2475_4349_B592_666A6B578701
#define F132A6BF_2475_4349_B592_666A6B578701

namespace style
{
struct Palette final {
    ImVec4 text;
    ImVec4 text_disabled;

    ImVec4 window;
    ImVec4 panel;
    ImVec4 popup;

    ImVec4 border;
    ImVec4 bevel_light;
    ImVec4 bevel_dark;

    ImVec4 button;
    ImVec4 button_hovered;
    ImVec4 button_active;
    ImVec4 button_disabled;

    ImVec4 frame;
    ImVec4 frame_hovered;
    ImVec4 frame_active;

    ImVec4 accent;
};
} // namespace style

namespace style
{
const Palette& palette(void);
} // namespace style

namespace style
{
void apply(void);
} // namespace style

#endif /* F132A6BF_2475_4349_B592_666A6B578701 */
