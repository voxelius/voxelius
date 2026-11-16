#ifndef D0DCF3F0_32A4_4C05_B9B0_3C6CD1FE3E3E
#define D0DCF3F0_32A4_4C05_B9B0_3C6CD1FE3E3E

#include "client/gui/widget.hh"

namespace gui
{
class CommandLine final : public WidgetBuilder<CommandLine> {
public:
    CommandLine& set_font(ImFont* font, float size = 0.0f);
    CommandLine& set_button_text(std::string_view key);
    CommandLine& set_history_size(std::size_t size);
    CommandLine& on_submit(std::function<void(std::string_view)> callback);

    void focus(void);
    void clear(void);

    virtual void layout(void) override;
    virtual void translate(void) override;

private:
    static int on_callback(ImGuiInputTextCallbackData* data);

    void submit(void);

    ImFont* m_font { nullptr };
    float m_font_size { 0.0f };

    std::string m_button_key {};
    std::string m_button_text {};
    std::string m_button_label {};

    std::string m_buffer {};

    std::size_t m_history_size { 64 };
    std::deque<std::string> m_history {};
    int m_history_pos { -1 };

    std::function<void(std::string_view)> m_submit {};

    bool m_focus { false };
    bool m_swallow { false };
};
} // namespace gui

#endif /* D0DCF3F0_32A4_4C05_B9B0_3C6CD1FE3E3E */
