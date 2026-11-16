#ifndef FB1EE509_C5A0_4418_A7A1_2F1DF093A4B5
#define FB1EE509_C5A0_4418_A7A1_2F1DF093A4B5

#include "core/msg/sink.hh"

#include "client/gui/widget.hh"

namespace gui
{
class LogView final : public WidgetBuilder<LogView> {
public:
    LogView& set_font(ImFont* font, float size = 0.0f);
    LogView& set_capacity(std::size_t capacity);
    LogView& set_padding(ImVec2 padding);

    void push(msg::level_type level, std::string text);
    void push_echo(std::string text);
    void clear(void);

    virtual void layout(void) override;

private:
    struct Entry final {
        std::optional<msg::level_type> level;
        std::string text;
    };

    void drain(void);
    void push_lines(std::optional<msg::level_type> level, std::string_view text);

    ImFont* m_font { nullptr };
    float m_font_size { 0.0f };
    ImVec2 m_padding { 3.0f, 2.0f };
    std::size_t m_capacity { 1024 };

    std::mutex m_mutex {};
    std::deque<Entry> m_pending {};
    bool m_clear_pending { false };

    std::deque<Entry> m_lines {};
};
} // namespace gui

#endif /* FB1EE509_C5A0_4418_A7A1_2F1DF093A4B5 */
