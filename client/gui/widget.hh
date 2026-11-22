#ifndef C4579E81_6921_4BD8_AC2C_9181D65F30D4
#define C4579E81_6921_4BD8_AC2C_9181D65F30D4

namespace gui
{
class Widget {
public:
    virtual ~Widget(void) = default;
    virtual void layout(void) = 0;
    virtual void translate(void);

    constexpr bool visible(void) const;
    void set_visible(bool visible);

protected:
    const std::string& imgui_id(void) const;

private:
    bool m_visible { true };
    mutable std::string m_imgui_id {};
    mutable const void* m_last_this { nullptr };
};
} // namespace gui

namespace gui
{
template<class Derived>
class WidgetBuilder : public Widget {
protected:
    constexpr Derived& self(void);
};
} // namespace gui

namespace gui
{
void update_scale(void);
} // namespace gui

constexpr bool gui::Widget::visible(void) const
{
    return m_visible;
}

template<class Derived>
constexpr Derived& gui::WidgetBuilder<Derived>::self(void)
{
    return static_cast<Derived&>(*this);
}

#endif /* C4579E81_6921_4BD8_AC2C_9181D65F30D4 */
