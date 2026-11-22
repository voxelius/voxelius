#include "client/pch.hh"

#include "client/gui/setting.hh"

#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/language.hh"

constexpr static float CONTROL_FRACTION = 0.5f;
constexpr static float ROW_PAD_X = 4.0f;
constexpr static float TOOLTIP_WRAP_EM = 40.0f;

// A stone face paints its bevel on top of the frame, and the stock padding is
// in physical pixels, so on a scaled display the glyphs end up on the bevel
constexpr static float CONTROL_PAD_Y = 4.0f;

void gui::detail::SettingWidget::layout(void)
{
    auto scale = static_cast<float>(globals::gui_scale);

    auto pad_x = ROW_PAD_X * scale;

    auto& style = ImGui::GetStyle();
    auto pad_y = std::max(style.FramePadding.y, CONTROL_PAD_Y * scale);

    // The label aligns itself to the frame padding, so this goes on before it
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, pad_y));

    auto row_start_x = ImGui::GetCursorPosX();
    auto content_size = ImGui::GetContentRegionAvail();
    auto row_min = ImGui::GetCursorScreenPos();

    auto control_width = CONTROL_FRACTION * content_size.x - pad_x;
    auto label_width = content_size.x - control_width - 2.0f * pad_x;

    auto draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);

    ImGui::SetCursorPosX(row_start_x + pad_x);
    ImGui::AlignTextToFramePadding();

    // Long translations wrap inside their column instead of running under
    // the control
    ImGui::PushTextWrapPos(row_start_x + pad_x + label_width);
    ImGui::TextUnformatted(m_label.c_str());
    ImGui::PopTextWrapPos();

    // The item rect is the only place that holds where the text really
    // landed: AlignTextToFramePadding moves it down at draw time
    auto label_min = ImGui::GetItemRectMin();
    auto label_max = ImGui::GetItemRectMax();

    // ImGui text cannot carry a shadow, so it goes to the lower channel: the
    // merge draws it first and the label lands on top of it
    auto shadow_offset = text_shadow_offset();
    auto shadow_color = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.750f));

    draw_list->ChannelsSetCurrent(0);
    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(label_min.x + shadow_offset, label_min.y + shadow_offset),
        shadow_color, m_label.c_str(), nullptr, label_width);
    draw_list->ChannelsSetCurrent(1);

    if(m_tooltip_enabled) {
        if(ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(TOOLTIP_WRAP_EM * ImGui::GetFontSize());
            ImGui::TextUnformatted(m_tooltip.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    // The control always sits on the first line of the row, even when the
    // label wrapped to a second one
    ImGui::SetNextItemWidth(control_width);
    ImGui::SetCursorScreenPos(ImVec2(row_min.x + content_size.x - control_width - pad_x, row_min.y));

    layout_control();

    auto control_max = ImGui::GetItemRectMax();
    auto row_max = ImVec2(row_min.x + content_size.x, std::max(label_max.y, control_max.y));

    auto row_pad_y = 0.5f * style.ItemSpacing.y;
    row_min.y -= row_pad_y;
    row_max.y += row_pad_y;

    draw_list->ChannelsSetCurrent(0);

    if(ImGui::IsMouseHoveringRect(row_min, row_max)) {
        draw_list->AddRectFilled(row_min, row_max, ImGui::GetColorU32(m_highlight));
    }

    draw_list->ChannelsMerge();

    // The cursor only followed the control, so a wrapped label would be
    // overlapped by the next row. A dummy item grows the row instead of
    // moving the cursor by hand, which ImGui does not allow past the edge
    auto extra = std::max(0.0f, label_max.y - control_max.y);

    if(extra > 0.0f) {
        ImGui::Dummy(ImVec2(1.0f, extra));
    }

    ImGui::PopStyleVar();
}

void gui::detail::SettingWidget::translate(void)
{
    m_label = language::resolve(std::format("settings.value.{}", m_key));
    m_tooltip = language::resolve(std::format("settings.tooltip.{}", m_key));
    translate_control();
}
