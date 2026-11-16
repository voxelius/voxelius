#ifndef EE142D39_DC8C_4D10_8B78_C611FA561C47
#define EE142D39_DC8C_4D10_8B78_C611FA561C47

#include "client/gui/widget.hh"

namespace gui
{
class Rail final : public WidgetBuilder<Rail> {
public:
    Rail& set_width(float width);
    Rail& set_callback(std::function<void(void)> callback);
    Rail& add_page(std::string_view label_key, Widget& content, int priority = 0);

    virtual void layout(void) override;
    virtual void translate(void) override;

private:
    struct Page final {
        std::string label_key;
        std::string label;
        Widget* content;
        int priority;
    };

    void select(std::size_t index);

    std::vector<Page> m_pages {};
    std::size_t m_selected { 0 };
    std::function<void(void)> m_callback {};
    std::string m_callback_label {};
    float m_width { 90.0f };
};
} // namespace gui

#endif /* EE142D39_DC8C_4D10_8B78_C611FA561C47 */
