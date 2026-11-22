#include "client/pch.hh"

#include "client/gui/widget.hh"

#include "client/constant.hh"
#include "client/globals.hh"

void gui::update_scale(void)
{
    int width, height;
    SDL_GetWindowSize(globals::window, &width, &height);

    auto scale_x = std::max(1.0f, std::floor(static_cast<float>(width) / static_cast<float>(constant::BASE_WIDTH)));
    auto scale_y = std::max(1.0f, std::floor(static_cast<float>(height) / static_cast<float>(constant::BASE_HEIGHT)));
    auto scale_min = std::min(scale_x, scale_y);

    // ImGui 1.92 wants FontScaleMain instead of the legacy
    // FontGlobalScale: only the former is visible to GetFontSize(),
    // so the widget metrics follow the text instead of staying a step behind
    auto& style = ImGui::GetStyle();
    style.FontScaleMain = scale_min;
    globals::gui_scale = static_cast<unsigned>(scale_min);
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
