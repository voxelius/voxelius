#ifndef FE73D78F_C153_4C43_84B9_F8D3C86FAA20
#define FE73D78F_C153_4C43_84B9_F8D3C86FAA20

#include "client/gui/container.hh"

namespace gui
{
class Scroller final : public ContainerBuilder<Scroller> {
public:
    Scroller& set_margin(ImVec2 margin);
    Scroller& set_scroll_speed(float speed);
    Scroller& enable_background(void);

    virtual void layout(void) override;

private:
    ImVec2 m_margin {};
    float m_scroll_speed { 32.0f };
    bool m_background { false };
};
} // namespace gui

#endif /* FE73D78F_C153_4C43_84B9_F8D3C86FAA20 */
