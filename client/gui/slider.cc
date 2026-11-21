#include "client/pch.hh"

#include "client/gui/slider.hh"

#include "core/cvar/host.hh"

#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/style.hh"

constexpr static float HANDLE_WIDTH = 7.0f;

static bool layout_slider(const std::string& id, std::string_view text, float& fraction, float snap_steps)
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

        // The handle shows the value that the slider will actually hold, so it
        // snaps to the steps of an integer range instead of gliding
        if(snap_steps > 0.0f) {
            fraction = std::round(fraction * snap_steps) / snap_steps;
        }
    }

    auto draw_list = ImGui::GetWindowDrawList();
    auto item_max = ImVec2(item_min.x + size.x, item_min.y + size.y);

    // The trough is inset: shadow on the top left, light on the bottom right
    draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.frame));
    draw_list->AddRect(item_min, item_max, ImGui::GetColorU32(palette.border));

    draw_list->AddRectFilled(item_min, ImVec2(item_max.x, item_min.y + bevel), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(item_min, ImVec2(item_min.x + bevel, item_max.y), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(ImVec2(item_min.x, item_max.y - bevel), item_max, ImGui::GetColorU32(palette.bevel_light));
    draw_list->AddRectFilled(ImVec2(item_max.x - bevel, item_min.y), item_max, ImGui::GetColorU32(palette.bevel_light));

    auto handle_min = ImVec2(item_min.x + fraction * travel, item_min.y + bevel);
    auto handle_max = ImVec2(handle_min.x + handle_width, item_max.y - bevel);

    // The part left of the handle reads as the filled one
    if(handle_min.x > item_min.x + bevel) {
        draw_list->AddRectFilled(ImVec2(item_min.x + bevel, item_min.y + bevel), ImVec2(handle_min.x, item_max.y - bevel),
            ImGui::GetColorU32(palette.button_active));

        // A dark seam keeps the handle readable even when it sits far right
        draw_list->AddRectFilled(ImVec2(handle_min.x - 1.0f, item_min.y + bevel), ImVec2(handle_min.x, item_max.y - bevel),
            ImGui::GetColorU32(palette.bevel_dark));
    }

    // The handle is a stone knob rather than a flat bar, so it reads as
    // something you can grab even at the small widget size
    gui::detail::draw_stone_face(draw_list, handle_min, handle_max, palette.bevel_light);

    auto shadow_color = ImGui::GetColorU32(palette.bevel_dark);
    auto text_color = ImGui::GetColorU32(palette.text);

    auto text_size = ImGui::CalcTextSize(text.data(), text.data() + text.size());

    auto text_position = ImVec2();
    text_position.x = std::floor(item_min.x + 0.5f * (size.x - text_size.x));
    text_position.y = std::floor(item_min.y + 0.5f * (size.y - text_size.y));

    draw_list->AddText(ImVec2(text_position.x + 1.0f, text_position.y + 1.0f), shadow_color, text.data(), text.data() + text.size());
    draw_list->AddText(text_position, text_color, text.data(), text.data() + text.size());

    return active;
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

    if(layout_slider(this->imgui_id(), buffer, fraction, static_cast<float>(range))) {
        auto next = m_min + static_cast<int>(std::lround(fraction * static_cast<float>(range)));
        m_value.set_value(std::clamp(next, m_min, m_max));
    }
}

template<>
void gui::Slider<float>::layout_control(void)
{
    auto current = m_value.value();

    auto fraction = 0.0f;

    if(m_max > m_min) {
        fraction = (current - m_min) / (m_max - m_min);
    }

    char buffer[64];
    auto format = "%.3f";

    if(!m_format.empty()) {
        format = m_format.c_str();
    }

    std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(current));

    if(layout_slider(this->imgui_id(), buffer, fraction, 0.0f)) {
        m_value.set_value(std::clamp(m_min + fraction * (m_max - m_min), m_min, m_max));
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

    if(layout_slider(this->imgui_id(), buffer, fraction, static_cast<float>(range))) {
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
