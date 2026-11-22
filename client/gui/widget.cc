#include "client/pch.hh"

#include "client/gui/widget.hh"

#include "client/constant.hh"
#include "client/globals.hh"

void gui::update_scale(void)
{
    int width, height;
    SDL_GetWindowSize(globals::window, &width, &height);

    auto half_base_width = 0.5f * static_cast<float>(constant::BASE_WIDTH);
    auto half_base_height = 0.5f * static_cast<float>(constant::BASE_HEIGHT);

    auto twice_scale_x = static_cast<float>(width) / half_base_width;
    auto twice_scale_y = static_cast<float>(height) / half_base_height;

    auto scale_x = std::max(1.0f, 0.5f * std::floor(twice_scale_x));
    auto scale_y = std::max(1.0f, 0.5f * std::floor(twice_scale_y));
    auto raw_scale = std::min(scale_x, scale_y);
    auto scale_min = std::ceil(raw_scale);

    // Even scales keep the pixel fonts on their design grid
    auto scale_even = std::max(1.0f, std::floor(0.5f * scale_min) * 2.0f);
    auto scale = scale_even;

    // A jump up needs a full base unit of width to spare, or the UI feels cramped
    if(scale > raw_scale && static_cast<float>(width) < (scale + 1.0f) * static_cast<float>(constant::BASE_WIDTH)) {
        scale -= 1.0f;
    }

    // ImGui 1.92 wants FontScaleMain instead of the legacy
    // FontGlobalScale: only the former is visible to GetFontSize(),
    // so the widget metrics follow the text instead of staying a step behind
    auto& style = ImGui::GetStyle();
    style.FontScaleMain = scale;
    globals::gui_scale = static_cast<unsigned>(scale);
}

void gui::Widget::translate(void)
{
    // empty
}

void gui::Widget::set_visible(bool visible)
{
    m_visible = visible;
}

const std::string& gui::Widget::imgui_id(void) const
{
    if(this == m_last_this) {
        return m_imgui_id;
    }

    m_last_this = this;
    m_imgui_id = std::format("###{}", static_cast<const void*>(this));
    return m_imgui_id;
}
