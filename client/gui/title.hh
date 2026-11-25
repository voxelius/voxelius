#ifndef F620C710_7810_4EEA_A6FD_45C5EAC6CDBB
#define F620C710_7810_4EEA_A6FD_45C5EAC6CDBB

#include "client/gui/widget.hh"

namespace gui
{
class Title final : public WidgetBuilder<Title> {
public:
    Title& set_texture(ImTextureID texture, ImVec2 size);
    Title& set_splash(std::string_view text);
    Title& set_version(std::string_view text);
    Title& set_top(float top);
    Title& set_width(float width);

    virtual void layout(void) override;

private:
    void layout_header(ImVec2 window_size, ImDrawList* draw_list, float shadow_offset);

    ImTextureID m_texture { 0 };
    ImVec2 m_texture_size {};
    std::string m_splash {};
    std::string m_version {};
    float m_top { 0.06f };
    float m_width_fraction { 0.73f };
};
} // namespace gui

#endif /* F620C710_7810_4EEA_A6FD_45C5EAC6CDBB */
