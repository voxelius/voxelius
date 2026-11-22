#include "client/pch.hh"

#include "client/gui/loading_panel.hh"

#include "client/fonts.hh"
#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/style.hh"

constexpr static float PANEL_WIDTH = 240.0f;
constexpr static float PANEL_PADDING = 10.0f;
constexpr static float GAP = 4.0f;
constexpr static float INSET_PADDING = 10.0f;
constexpr static float BAR_HEIGHT = 14.0f;
constexpr static float BUTTON_WIDTH = 80.0f;
constexpr static float BUTTON_HEIGHT = 16.0f;

constexpr static std::string_view CANCEL_KEY = "gui.progress_popup.cancel";

gui::LoadingPanel& gui::LoadingPanel::set_title(std::string_view key)
{
    m_title_key = key;

    return self();
}

gui::LoadingPanel& gui::LoadingPanel::set_status(std::string_view key)
{
    m_status_key = key;

    return self();
}

gui::LoadingPanel& gui::LoadingPanel::set_progress(std::optional<float> progress)
{
    m_progress = std::move(progress);

    return self();
}

gui::LoadingPanel& gui::LoadingPanel::set_cancel_visible(bool visible)
{
    m_cancel_visible = visible;

    return self();
}

gui::LoadingPanel& gui::LoadingPanel::on_cancel(std::function<void(void)> callback)
{
    m_cancel = std::move(callback);

    return self();
}

void gui::LoadingPanel::layout(void)
{
    auto& palette = style::palette();
    auto scale = static_cast<float>(globals::gui_scale);

    auto window_pos = ImGui::GetWindowPos();
    auto window_size = ImGui::GetWindowSize();

    auto padding = PANEL_PADDING * scale;
    auto gap = GAP * scale;
    auto inset_padding = INSET_PADDING * scale;
    auto bar_height = BAR_HEIGHT * scale;
    auto button_size = ImVec2(BUTTON_WIDTH * scale, BUTTON_HEIGHT * scale);

    ImGui::PushFont(fonts::unscii16, fonts::unscii16->LegacySize);
    auto title_height = ImGui::GetTextLineHeight();
    ImGui::PopFont();

    ImGui::PushFont(fonts::unscii8, fonts::unscii8->LegacySize);
    auto status_height = ImGui::GetTextLineHeight();
    ImGui::PopFont();

    auto inset_height = 2.0f * inset_padding + status_height + gap + bar_height;
    auto panel_width = PANEL_WIDTH * scale;
    auto panel_height = 2.0f * padding + title_height + gap + inset_height;

    if(m_cancel_visible) {
        panel_height += gap + button_size.y;
    }

    auto panel_min = ImVec2();
    panel_min.x = window_pos.x + 0.5f * (window_size.x - panel_width);
    panel_min.y = window_pos.y + 0.5f * (window_size.y - panel_height);

    auto panel_max = ImVec2(panel_min.x + panel_width, panel_min.y + panel_height);
    auto panel_center_x = 0.5f * (panel_min.x + panel_max.x);

    auto draw_list = ImGui::GetWindowDrawList();
    auto shadow_color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.85f));

    draw_list->AddRectFilled(panel_min, panel_max, ImGui::GetColorU32(palette.popup));
    draw_list->AddRect(panel_min, panel_max, ImGui::GetColorU32(palette.border));

    ImGui::PushFont(fonts::unscii16, fonts::unscii16->LegacySize);
    auto title_size = ImGui::CalcTextSize(m_title.c_str());

    auto title_position = ImVec2(panel_center_x - 0.5f * title_size.x, panel_min.y + padding);

    draw_list->AddText(ImVec2(title_position.x + scale, title_position.y + scale), shadow_color, m_title.c_str());
    ImGui::SetCursorScreenPos(title_position);
    ImGui::TextUnformatted(m_title.c_str());
    ImGui::PopFont();

    auto inset_min = ImVec2(panel_min.x + padding, panel_min.y + padding + title_height + gap);
    auto inset_max = ImVec2(panel_max.x - padding, inset_min.y + inset_height);

    draw_list->AddRectFilled(inset_min, inset_max, ImGui::GetColorU32(palette.frame));
    draw_list->AddRect(inset_min, inset_max, ImGui::GetColorU32(palette.border));

    ImGui::PushFont(fonts::unscii8, fonts::unscii8->LegacySize);
    auto status_size = ImGui::CalcTextSize(m_status.c_str());

    auto status_position = ImVec2(panel_center_x - 0.5f * status_size.x, inset_min.y + inset_padding);

    draw_list->AddText(ImVec2(status_position.x + scale, status_position.y + scale), shadow_color, m_status.c_str());
    ImGui::SetCursorScreenPos(status_position);
    ImGui::TextUnformatted(m_status.c_str());
    ImGui::PopFont();

    auto bar_width = (inset_max.x - inset_min.x) - 2.0f * inset_padding;
    auto bar_position = ImVec2(inset_min.x + inset_padding, inset_max.y - inset_padding - bar_height);
    auto progress = m_progress.value_or(-static_cast<float>(ImGui::GetTime()));

    ImGui::SetCursorScreenPos(bar_position);
    ImGui::ProgressBar(progress, ImVec2(bar_width, bar_height), "");

    if(m_cancel_visible) {
        auto button_position = ImVec2(panel_center_x - 0.5f * button_size.x, inset_max.y + gap);

        ImGui::SetCursorScreenPos(button_position);

        if(gui::detail::stone_button(imgui_id().c_str(), m_cancel_label, button_size)) {
            if(m_cancel) {
                m_cancel();
            }
        }
    }
}

void gui::LoadingPanel::translate(void)
{
    m_title = language::resolve(m_title_key);
    m_status = language::resolve(m_status_key);
    m_cancel_label = language::resolve(CANCEL_KEY);
}
