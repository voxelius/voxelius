#include "client/pch.hh"

#include "client/gui/panel.hh"

#include "client/globals.hh"
#include "client/style.hh"

constexpr static float PANEL_PADDING = 8.0f;

gui::Panel& gui::Panel::set_margin(ImVec2 margin)
{
    m_margin = std::move(margin);

    return self();
}

gui::Panel& gui::Panel::enable_background(void)
{
    m_background = true;

    return self();
}

void gui::Panel::layout(void)
{
    ImVec2 margin {};
    margin.x = m_margin.x * static_cast<float>(globals::gui_scale);
    margin.y = m_margin.y * static_cast<float>(globals::gui_scale);

    auto avail = ImGui::GetContentRegionAvail();

    ImVec2 size {};
    size.x = avail.x - 2.0f * margin.x;
    size.y = avail.y - 2.0f * margin.y;

    auto cursor_pos = ImGui::GetCursorPos();
    cursor_pos.x += margin.x;
    cursor_pos.y += margin.y;

    ImGui::SetCursorPos(cursor_pos);

    if(m_background) {
        auto& palette = style::palette();

        auto panel_min = ImGui::GetCursorScreenPos();
        auto panel_max = ImVec2(panel_min.x + size.x, panel_min.y + size.y);

        auto draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(panel_min, panel_max, ImGui::GetColorU32(palette.panel));
        draw_list->AddRect(panel_min, panel_max, ImGui::GetColorU32(palette.border));
    }

    auto& window_id = imgui_id();
    auto window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;

    auto child_flags = m_background ? ImGuiChildFlags_AlwaysUseWindowPadding : ImGuiChildFlags_None;

    if(m_background) {
        auto padding = PANEL_PADDING * static_cast<float>(globals::gui_scale);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    }

    if(ImGui::BeginChild(window_id.c_str(), size, child_flags, window_flags)) {
        ContainerBuilder<Panel>::layout();
    }

    ImGui::EndChild();

    if(m_background) {
        ImGui::PopStyleVar();
    }
}
