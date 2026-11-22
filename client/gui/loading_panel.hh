#ifndef AAE527FE_54FF_492D_84DC_BC673B6D834F
#define AAE527FE_54FF_492D_84DC_BC673B6D834F

#include "client/gui/widget.hh"

namespace gui
{
class LoadingPanel final : public WidgetBuilder<LoadingPanel> {
public:
    LoadingPanel& set_title(std::string_view key);
    LoadingPanel& set_status(std::string_view key);
    LoadingPanel& set_progress(std::optional<float> progress);
    LoadingPanel& set_cancel_visible(bool visible);
    LoadingPanel& on_cancel(std::function<void(void)> callback);

    virtual void layout(void) override;
    virtual void translate(void) override;

private:
    std::optional<float> m_progress {};
    std::function<void(void)> m_cancel {};
    bool m_cancel_visible { true };
    std::string m_title_key { "gui.loading.title" };
    std::string m_status_key { "gui.loading.status" };
    std::string m_title {};
    std::string m_status {};
    std::string m_cancel_label {};
};
} // namespace gui

#endif /* AAE527FE_54FF_492D_84DC_BC673B6D834F */
