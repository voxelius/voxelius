#ifndef B7C4E2A1_5F3D_4B8A_9C6E_2D1F0A3B4C5D
#define B7C4E2A1_5F3D_4B8A_9C6E_2D1F0A3B4C5D

#include "client/gui/setting.hh"
#include "client/input/source.hh"

struct Texture2D;

namespace gui
{
class ButtonBind final : public SettingBuilder<ButtonBind> {
public:
    static void init(void);
    static void shutdown(void);

    static ButtonBind* current;

    ButtonBind& bind_action(std::string_view action, unsigned slot = 0);
    ButtonBind& bind_command(std::string_view command, input_edge edge = input_edge::DOWN);

protected:
    virtual void layout_control(void) override;
    virtual void translate_control(void) override;

private:
    static void on_keyboard_event(const SDL_KeyboardEvent& event);
    static void on_mouse_button_event(const SDL_MouseButtonEvent& event);
    static void on_mouse_wheel_event(const SDL_MouseWheelEvent& event);

    void capture(std::optional<ButtonSource> source);
    std::optional<ButtonSource> bound_button(void) const;

    std::string m_press {};
    std::string m_release {};
    unsigned m_slot {};
    input_edge m_edge { input_edge::DOWN };
    bool m_is_action { true };
    std::string m_capture_label {};
    std::string m_bound_label {};
};
} // namespace gui

#endif /* B7C4E2A1_5F3D_4B8A_9C6E_2D1F0A3B4C5D */
