#ifndef EE73069F_7AB6_41A9_9999_3BE1F73D3DDC
#define EE73069F_7AB6_41A9_9999_3BE1F73D3DDC

#include "client/gui/container.hh"

namespace gui
{
class Screen final : public ContainerBuilder<Screen> {
public:
    Screen& set_start(float xrel, float yrel);
    Screen& set_size(float xrel, float yrel);
    Screen& set_flags(ImGuiWindowFlags flags);

    virtual void layout(void) override;

private:
    ImVec2 m_start {};
    ImVec2 m_size { 1.0f, 1.0f };
    ImGuiWindowFlags m_flags { ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration };
};
} // namespace gui

#endif /* EE73069F_7AB6_41A9_9999_3BE1F73D3DDC */
