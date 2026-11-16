#ifndef C3D5E7F9_1B2A_4C6D_8E0F_2A4B6C8D0E1F
#define C3D5E7F9_1B2A_4C6D_8E0F_2A4B6C8D0E1F

#include "client/gui/stone.hh"
#include "client/gui/widget.hh"

namespace gui
{
class MenuButton final : public WidgetBuilder<MenuButton> {
public:
    MenuButton& set_icon(icon_type icon);
    MenuButton& add_item(std::string_view key, std::function<void(void)> callback);

    virtual void layout(void) override;
    virtual void translate(void) override;

private:
    struct Item final {
        std::string key;
        std::string label;
        std::function<void(void)> callback;
    };

    icon_type m_icon { icon_type::CHEVRON };
    std::vector<Item> m_items {};
};
} // namespace gui

#endif /* C3D5E7F9_1B2A_4C6D_8E0F_2A4B6C8D0E1F */
