#ifndef E391CB4E_D5D8_4BE6_B59D_895AE580F3E3
#define E391CB4E_D5D8_4BE6_B59D_895AE580F3E3

#include "client/gui/container.hh"

namespace gui
{
class Panel final : public ContainerBuilder<Panel> {
public:
    Panel& set_margin(ImVec2 margin);
    Panel& enable_background(void);

    virtual void layout(void) override;

private:
    ImVec2 m_margin {};
    bool m_background { false };
};
} // namespace gui

#endif /* E391CB4E_D5D8_4BE6_B59D_895AE580F3E3 */
