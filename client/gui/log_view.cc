#include "client/pch.hh"

#include "client/gui/log_view.hh"

#include "client/globals.hh"

constexpr static ImVec4 COLOR_ECHO = ImVec4(0.700f, 0.700f, 0.700f, 1.000f);
constexpr static ImVec4 COLOR_INFO = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
constexpr static ImVec4 COLOR_WARNING = ImVec4(1.000f, 0.900f, 0.400f, 1.000f);
constexpr static ImVec4 COLOR_ERROR = ImVec4(1.000f, 0.400f, 0.350f, 1.000f);
constexpr static ImVec4 COLOR_CRITICAL = ImVec4(1.000f, 0.200f, 0.200f, 1.000f);
constexpr static ImVec4 COLOR_DEBUG = ImVec4(0.500f, 0.850f, 1.000f, 1.000f);
constexpr static ImVec4 COLOR_UNKNOWN = ImVec4(1.000f, 0.400f, 1.000f, 1.000f);

static const ImVec4& entry_color(const std::optional<msg::level_type>& level)
{
    if(!level.has_value()) {
        return COLOR_ECHO;
    }

    switch(level.value()) {
        case msg::LEVEL_INFO:
            return COLOR_INFO;

        case msg::LEVEL_WARNING:
            return COLOR_WARNING;

        case msg::LEVEL_ERROR:
            return COLOR_ERROR;

        case msg::LEVEL_CRITICAL:
            return COLOR_CRITICAL;

        case msg::LEVEL_DEBUG:
            return COLOR_DEBUG;

        default:
            return COLOR_UNKNOWN;
    }
}

gui::LogView& gui::LogView::set_font(ImFont* font, float size)
{
    m_font = font;
    m_font_size = size;

    return self();
}

gui::LogView& gui::LogView::set_padding(ImVec2 padding)
{
    m_padding = std::move(padding);

    return self();
}

gui::LogView& gui::LogView::set_capacity(std::size_t capacity)
{
    std::lock_guard locker(m_mutex);

    m_capacity = std::max<std::size_t>(capacity, 1);

    while(m_pending.size() > m_capacity) {
        m_pending.pop_front();
    }

    return self();
}

void gui::LogView::push(msg::level_type level, std::string text)
{
    std::lock_guard locker(m_mutex);

    push_lines(level, text);
}

void gui::LogView::push_echo(std::string text)
{
    std::lock_guard locker(m_mutex);

    push_lines(std::nullopt, text);
}

void gui::LogView::push_lines(std::optional<msg::level_type> level, std::string_view text)
{
    std::size_t start = 0;

    while(true) {
        std::string_view line;

        auto pos = text.find('\n', start);

        if(pos == std::string_view::npos) {
            line = text.substr(start);
        }
        else {
            line = text.substr(start, pos - start);
        }

        if(line.ends_with('\r')) {
            line.remove_suffix(1);
        }

        Entry entry {};
        entry.level = level;
        entry.text = std::string(line);

        m_pending.emplace_back(std::move(entry));

        if(pos == std::string_view::npos) {
            break;
        }

        start = pos + 1;
    }

    while(m_pending.size() > m_capacity) {
        m_pending.pop_front();
    }
}

void gui::LogView::clear(void)
{
    std::lock_guard locker(m_mutex);

    m_pending.clear();
    m_clear_pending = true;
}

void gui::LogView::drain(void)
{
    std::lock_guard locker(m_mutex);

    if(m_clear_pending) {
        m_lines.clear();
        m_clear_pending = false;
    }

    for(auto& entry : m_pending) {
        m_lines.emplace_back(std::move(entry));
    }

    m_pending.clear();

    while(m_lines.size() > m_capacity) {
        m_lines.pop_front();
    }
}

void gui::LogView::layout(void)
{
    drain();

    ImVec2 padding {};
    padding.x = m_padding.x * static_cast<float>(globals::gui_scale);
    padding.y = m_padding.y * static_cast<float>(globals::gui_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);

    auto child_flags = ImGuiChildFlags_AlwaysUseWindowPadding;
    auto window_flags = ImGuiWindowFlags_NoBackground;

    if(ImGui::BeginChild(imgui_id().c_str(), ImVec2(0.0f, 0.0f), child_flags, window_flags)) {
        if(m_font) {
            auto size = m_font->LegacySize;

            if(m_font_size > 0.0f) {
                auto font_scale = std::max(1.0f, ImGui::GetStyle().FontScaleMain);
                auto font_steps = std::max(1.0f, std::floor(font_scale / 2.0f));
                size = m_font_size * font_steps / font_scale;
            }

            ImGui::PushFont(m_font, size);
        }

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_lines.size()));

        while(clipper.Step()) {
            for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                const auto& entry = m_lines[static_cast<std::size_t>(i)];

                ImGui::PushStyleColor(ImGuiCol_Text, entry_color(entry.level));
                ImGui::TextUnformatted(entry.text.c_str());
                ImGui::PopStyleColor();
            }
        }

        clipper.End();

        if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        if(m_font) {
            ImGui::PopFont();
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();
}
