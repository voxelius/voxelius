#ifndef D8BDD832_27CE_404A_8ADD_6C4BEFC72B65
#define D8BDD832_27CE_404A_8ADD_6C4BEFC72B65

#include "client/gui/widget.hh"

namespace gui
{
using stack_policy_type = unsigned short;
constexpr static stack_policy_type FIXED = 0;
constexpr static stack_policy_type EXPANDING = 1;
} // namespace gui

namespace gui
{
class HorizontalStack final : public WidgetBuilder<HorizontalStack> {
public:
    HorizontalStack& add_item(Widget& widget, stack_policy_type policy = FIXED, float fixed_width = 0.0f);

    virtual void translate(void) override;
    virtual void layout(void) override;

private:
    struct Item final {
        Widget* widget;
        stack_policy_type policy;
        float fixed_width;
    };

    std::vector<Item> m_items {};
};
} // namespace gui

namespace gui
{
class VerticalStack final : public WidgetBuilder<VerticalStack> {
public:
    VerticalStack& add_item(Widget& widget, stack_policy_type policy = FIXED, float fixed_height = 0.0f);

    virtual void layout(void) override;
    virtual void translate(void) override;

private:
    struct Item final {
        Widget* widget;
        stack_policy_type policy;
        float fixed_height;
    };

    std::vector<Item> m_items {};
};
} // namespace gui

#endif /* D8BDD832_27CE_404A_8ADD_6C4BEFC72B65 */
