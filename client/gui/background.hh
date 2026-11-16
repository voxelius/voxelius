#ifndef C009E778_A0FE_4967_BD62_FBF417A19895
#define C009E778_A0FE_4967_BD62_FBF417A19895

#include "client/gui/widget.hh"

namespace gui
{
class Background final : public WidgetBuilder<Background> {
public:
    Background& set_left(const ImVec4& color);
    Background& set_right(const ImVec4& color);
    Background& set_texture(ImTextureID texture);

    Background& enable_scene(void);

    virtual void layout(void) override;

private:
    void layout_scene(ImVec2 window_pos, ImVec2 window_size);
    void layout_dim(ImVec2 window_pos, ImVec2 window_size);

    ImVec4 m_left { 0.0f, 0.0f, 0.0f, 0.95f };
    ImVec4 m_right { 0.0f, 0.0f, 0.0f, 0.95f };
    ImTextureID m_texture { 0 };
    bool m_scene { false };
};
} // namespace gui

#endif /* C009E778_A0FE_4967_BD62_FBF417A19895 */
