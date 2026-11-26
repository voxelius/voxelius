#include "client/pch.hh"

#include "client/gui/slider.hh"

#include "core/cvar/host.hh"

#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/style.hh"

constexpr static float HANDLE_WIDTH = 7.0f;

template<typename Snap>
static bool layout_slider(const std::string& id, std::string_view text, float& fraction, float min, float max, Snap snap)
{
    auto& palette = style::palette();

    auto width = ImGui::CalcItemWidth();
    auto height = ImGui::GetFrameHeight();

    auto size = ImVec2(width, height);
    auto item_min = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(id.c_str(), size);

    auto active = ImGui::IsItemActive();

    auto scale = static_cast<float>(globals::gui_scale);
    auto bevel = std::max(1.0f, scale);
    auto handle_width = std::max(2.0f, HANDLE_WIDTH * scale);
    auto travel = std::max(1.0f, size.x - handle_width);

    if(active) {
        auto mouse_x = ImGui::GetIO().MousePos.x;
        fraction = std::clamp((mouse_x - item_min.x - 0.5f * handle_width) / travel, 0.0f, 1.0f);

        // The handle shows the value that the slider will hold, so the value
        // is snapped first and the handle follows it instead of the cursor
        auto range = max - min;

        if(range > 0.0f) {
            auto value = snap(min + fraction * range);

            fraction = std::clamp((value - min) / range, 0.0f, 1.0f);
        }
    }

    auto draw_list = ImGui::GetWindowDrawList();
    auto item_max = ImVec2(item_min.x + size.x, item_min.y + size.y);

    draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.frame));
    draw_list->AddRect(item_min, item_max, ImGui::GetColorU32(palette.border));

    draw_list->AddRectFilled(item_min, ImVec2(item_max.x, item_min.y + bevel), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(item_min, ImVec2(item_min.x + bevel, item_max.y), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(ImVec2(item_min.x, item_max.y - bevel), item_max, ImGui::GetColorU32(palette.bevel_light));
    draw_list->AddRectFilled(ImVec2(item_max.x - bevel, item_min.y), item_max, ImGui::GetColorU32(palette.bevel_light));

    auto handle_min = ImVec2(item_min.x + fraction * travel, item_min.y + bevel);
    auto handle_max = ImVec2(handle_min.x + handle_width, item_max.y - bevel);

    if(handle_min.x > item_min.x + bevel) {
        draw_list->AddRectFilled(ImVec2(item_min.x + bevel, item_min.y + bevel), ImVec2(handle_min.x, item_max.y - bevel),
            ImGui::GetColorU32(palette.button_active));

        draw_list->AddRectFilled(ImVec2(handle_min.x - 1.0f, item_min.y + bevel), ImVec2(handle_min.x, item_max.y - bevel),
            ImGui::GetColorU32(palette.bevel_dark));
    }

    gui::detail::draw_stone_face(draw_list, handle_min, handle_max, palette.bevel_light);

    auto shadow_color = ImGui::GetColorU32(palette.bevel_dark);
    auto text_color = ImGui::GetColorU32(palette.text);

    auto text_size = ImGui::CalcTextSize(text.data(), text.data() + text.size());

    auto text_position = ImVec2();
    text_position.x = std::floor(item_min.x + 0.5f * (size.x - text_size.x));
    text_position.y = std::floor(item_min.y + 0.5f * (size.y - text_size.y));

    auto shadow_offset = gui::detail::text_shadow_offset();
    auto shadow_position = ImVec2(text_position.x + shadow_offset, text_position.y + shadow_offset);

    draw_list->AddText(shadow_position, shadow_color, text.data(), text.data() + text.size());
    draw_list->AddText(text_position, text_color, text.data(), text.data() + text.size());

    return active;
}

static float snap_to_whole(float value)
{
    return std::round(value);
}

// The value that the label prints, so the handle and the stored number
// follow the text that the user reads instead of the raw position
static float round_to_format(const char* format, float value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(value));

    auto text = std::string_view(buffer);
    auto offset = text.find_first_not_of(' ');

    if(offset == std::string_view::npos) {
        return value;
    }

    text.remove_prefix(offset);

    if(text.front() == '+') {
        text.remove_prefix(1);
    }

    auto result = 0.0f;
    auto check = std::from_chars(text.data(), text.data() + text.size(), result);

    if(check.ec != std::errc {}) {
        return value;
    }

    return result;
}

template<typename T>
gui::Slider<T>& gui::Slider<T>::bind(std::string_view name)
{
    this->set_key(name);

    cvar::bind_var(m_value, name);

    return this->self();
}

template<typename T>
gui::Slider<T>& gui::Slider<T>::set_range(T min, T max)
{
    m_min = min;
    m_max = max;

    return this->self();
}

template<typename T>
gui::Slider<T>& gui::Slider<T>::set_format(std::string_view format)
{
    m_format = format;

    return this->self();
}

template<typename T>
gui::Slider<T>& gui::Slider<T>::set_step(T step)
{
    m_step = step;

    return this->self();
}

template<typename T>
T gui::Slider<T>::value(void) const
{
    return m_value.value();
}

template<typename T>
void gui::Slider<T>::set_value(T value)
{
    m_value.set_value(value);
}

template<>
void gui::Slider<int>::layout_control(void)
{
    auto current = m_value.value();
    auto range = m_max - m_min;

    auto fraction = 0.0f;

    if(range) {
        fraction = static_cast<float>(current - m_min) / static_cast<float>(range);
    }

    char buffer[64];
    auto format = "%d";

    if(!m_format.empty()) {
        format = m_format.c_str();
    }

    std::snprintf(buffer, sizeof(buffer), format, current);

    auto min = static_cast<float>(m_min);
    auto max = static_cast<float>(m_max);

    if(layout_slider(this->imgui_id(), buffer, fraction, min, max, &snap_to_whole)) {
        auto next = m_min + static_cast<int>(std::lround(fraction * static_cast<float>(range)));

        m_value.set_value(std::clamp(next, m_min, m_max));
    }
}

template<>
void gui::Slider<float>::layout_control(void)
{
    auto current = m_value.value();

    auto range = m_max - m_min;
    auto fraction = 0.0f;

    if(range > 0.0f) {
        fraction = (current - m_min) / range;
    }

    char buffer[64];
    auto format = "%.3f";

    if(!m_format.empty()) {
        format = m_format.c_str();
    }

    std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(current));

    auto snap = [this, format](float value) {
        if(m_step > 0.0f) {
            return m_min + m_step * std::round((value - m_min) / m_step);
        }

        return round_to_format(format, value);
    };

    if(layout_slider(this->imgui_id(), buffer, fraction, m_min, m_max, snap)) {
        auto next = snap(m_min + fraction * range);

        m_value.set_value(std::clamp(next, m_min, m_max));
    }
}

template<>
void gui::Slider<unsigned>::layout_control(void)
{
    auto current = m_value.value();

    auto range = static_cast<std::uint32_t>(m_max) - static_cast<std::uint32_t>(m_min);
    auto fraction = 0.0f;

    if(range) {
        fraction = static_cast<float>(current - m_min) / static_cast<float>(range);
    }

    char buffer[64];
    auto format = "%u";

    if(!m_format.empty()) {
        format = m_format.c_str();
    }

    std::snprintf(buffer, sizeof(buffer), format, current);

    auto min = static_cast<float>(m_min);
    auto max = static_cast<float>(m_max);

    if(layout_slider(this->imgui_id(), buffer, fraction, min, max, &snap_to_whole)) {
        auto next = static_cast<std::uint32_t>(m_min) + static_cast<std::uint32_t>(std::lround(fraction * static_cast<float>(range)));

        m_value.set_value(std::min(next, static_cast<std::uint32_t>(m_max)));
    }
}

template<typename T>
void gui::Slider<T>::translate_control(void)
{
    // empty
}

template class gui::Slider<int>;
template class gui::Slider<float>;
template class gui::Slider<unsigned>;
