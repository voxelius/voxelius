#include "client/pch.hh"

#include "client/style.hh"

// Blocky dark stone: everything is a flat slab with a one pixel
// bevel. Light goes to the top left, shadow to the bottom right
static const style::Palette PALETTE = {
    .text = ImVec4(0.929f, 0.929f, 0.929f, 1.000f),
    .text_disabled = ImVec4(0.549f, 0.549f, 0.549f, 1.000f),

    .window = ImVec4(0.157f, 0.157f, 0.165f, 0.960f),
    .panel = ImVec4(0.220f, 0.220f, 0.227f, 1.000f),
    .popup = ImVec4(0.196f, 0.196f, 0.204f, 0.980f),

    .border = ImVec4(0.000f, 0.000f, 0.000f, 0.900f),
    .bevel_light = ImVec4(0.650f, 0.650f, 0.650f, 1.000f),
    .bevel_dark = ImVec4(0.094f, 0.094f, 0.094f, 1.000f),

    .button = ImVec4(0.500f, 0.500f, 0.500f, 1.000f),
    .button_hovered = ImVec4(0.590f, 0.590f, 0.590f, 1.000f),
    .button_active = ImVec4(0.430f, 0.430f, 0.430f, 1.000f),
    .button_disabled = ImVec4(0.340f, 0.340f, 0.340f, 1.000f),

    .frame = ImVec4(0.137f, 0.137f, 0.145f, 1.000f),
    .frame_hovered = ImVec4(0.180f, 0.180f, 0.188f, 1.000f),
    .frame_active = ImVec4(0.243f, 0.243f, 0.251f, 1.000f),

    .accent = ImVec4(0.918f, 0.918f, 0.918f, 1.000f),
};

static ImVec4 with_alpha(const ImVec4& color, float alpha)
{
    return ImVec4(color.x, color.y, color.z, alpha);
}

const style::Palette& style::palette(void)
{
    return PALETTE;
}

void style::apply(void)
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    auto& pal = palette();

    // Black buttons on a dark background
    // may be harder to read than the text on them
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowBorderSize = 0.0f;

    // Controls read better when they are a bit taller than the text
    // itself; this is in physical pixels, so it does not follow the scale
    style.FramePadding = ImVec2(6.0f, 5.0f);

    // Rounding on elements looks cool but I am
    // aiming for a more or less blocky and
    // visually simple HiDPI-friendly UI style
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowRounding = 0.0f;

    auto& colors = style.Colors;

    colors[ImGuiCol_Border] = pal.border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Button] = pal.button;
    colors[ImGuiCol_ButtonActive] = pal.button_active;
    colors[ImGuiCol_ButtonHovered] = pal.button_hovered;
    colors[ImGuiCol_CheckMark] = pal.accent;
    colors[ImGuiCol_ChildBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.420f);
    colors[ImGuiCol_DragDropTarget] = pal.accent;
    colors[ImGuiCol_FrameBg] = pal.frame;
    colors[ImGuiCol_FrameBgActive] = pal.frame_active;
    colors[ImGuiCol_FrameBgHovered] = pal.frame_hovered;

    // Selected list rows darken instead of lighting up: on a stone panel a
    // light fill swallows the secondary text of the row
    colors[ImGuiCol_Header] = ImVec4(0.000f, 0.000f, 0.000f, 0.620f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.000f, 0.000f, 0.000f, 0.760f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.070f);
    colors[ImGuiCol_MenuBarBg] = pal.panel;
    colors[ImGuiCol_NavCursor] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.69f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PopupBg] = pal.popup;
    colors[ImGuiCol_ResizeGrip] = with_alpha(pal.bevel_light, 0.20f);
    colors[ImGuiCol_ResizeGripActive] = pal.accent;
    colors[ImGuiCol_ResizeGripHovered] = with_alpha(pal.bevel_light, 0.67f);
    colors[ImGuiCol_ScrollbarBg] = pal.frame;
    colors[ImGuiCol_ScrollbarGrab] = pal.button;
    colors[ImGuiCol_ScrollbarGrabActive] = pal.button_active;
    colors[ImGuiCol_ScrollbarGrabHovered] = pal.button_hovered;
    colors[ImGuiCol_Separator] = with_alpha(pal.bevel_light, 0.50f);
    colors[ImGuiCol_SeparatorActive] = pal.accent;
    colors[ImGuiCol_SeparatorHovered] = with_alpha(pal.bevel_light, 0.78f);
    colors[ImGuiCol_SliderGrab] = with_alpha(pal.accent, 0.75f);
    colors[ImGuiCol_SliderGrabActive] = pal.accent;
    colors[ImGuiCol_Tab] = pal.button;
    colors[ImGuiCol_TabActive] = pal.button_active;
    colors[ImGuiCol_TabHovered] = pal.button_hovered;
    colors[ImGuiCol_TableBorderLight] = with_alpha(pal.bevel_light, 0.25f);
    colors[ImGuiCol_TableBorderStrong] = with_alpha(pal.bevel_light, 0.45f);
    colors[ImGuiCol_TableHeaderBg] = pal.button_active;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TabUnfocused] = pal.window;
    colors[ImGuiCol_TabUnfocusedActive] = pal.button_active;
    colors[ImGuiCol_Text] = pal.text;
    colors[ImGuiCol_TextDisabled] = pal.text_disabled;
    colors[ImGuiCol_TextSelectedBg] = with_alpha(pal.accent, 0.35f);
    colors[ImGuiCol_TitleBg] = pal.window;
    colors[ImGuiCol_TitleBgActive] = pal.panel;
    colors[ImGuiCol_TitleBgCollapsed] = with_alpha(pal.window, 0.51f);
    colors[ImGuiCol_WindowBg] = pal.window;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
}
