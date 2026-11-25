#ifndef A2C4B6F1_9A3D_4E57_8B21_6D0F5A7C3E94
#define A2C4B6F1_9A3D_4E57_8B21_6D0F5A7C3E94

namespace gui
{
// Drawn instead of a glyph: unscii has no pencil or refresh arrow
enum class icon_type {
    PLUS,
    MINUS,
    PENCIL,
    REFRESH,
    CHEVRON,
};
} // namespace gui

namespace gui::detail
{
// Loads the grain texture; called once at startup, the counterpart of stone_shutdown
void stone_init(void);
void stone_shutdown(void);
} // namespace gui::detail

namespace gui::detail
{
void draw_stone_face(ImDrawList* draw_list, ImVec2 item_min, ImVec2 item_max, const ImVec4& base);
void draw_stone_label(ImDrawList* draw_list, ImVec2 item_min, ImVec2 item_max, std::string_view label, bool pressed, bool enabled);
} // namespace gui::detail

namespace gui::detail
{
// One design pixel of the pixel fonts, in screen pixels
float text_shadow_offset(void);
} // namespace gui::detail

namespace gui::detail
{
// The label may carry an ImGui "###id" suffix; it is used for the item id and cut off before drawing
bool stone_button(const char* id, std::string_view label, ImVec2 size);
bool stone_arrow_button(const char* id, ImVec2 size);
} // namespace gui::detail

namespace gui::detail
{
void draw_icon(ImDrawList* draw_list, icon_type icon, ImVec2 item_min, ImVec2 item_max, ImU32 color);
} // namespace gui::detail

namespace gui::detail
{
// Opens the list of a combo or a stepper under the control that spawned it,
// without the frame and the padding of a regular window
bool stone_popup_begin(const char* id, ImVec2 item_min, ImVec2 item_max);
void stone_popup_end(bool open);
} // namespace gui::detail

namespace gui::detail
{
// Draws the field of a combo box and returns true when it was clicked;
// the caller opens the popup with the same id
bool stone_combo(const char* id, std::string_view label, ImVec2 size);
} // namespace gui::detail

#endif /* A2C4B6F1_9A3D_4E57_8B21_6D0F5A7C3E94 */
