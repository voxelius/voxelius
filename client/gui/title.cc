#include "client/pch.hh"

#include "client/gui/title.hh"

#include "client/fonts.hh"
#include "client/globals.hh"

constexpr static float SPLASH_GAP = 4.0f;
constexpr static float VERSION_PAD = 4.0f;

gui::Title& gui::Title::set_texture(ImTextureID texture, ImVec2 size)
{
    m_texture = texture;
    m_texture_size = size;

    return self();
}

gui::Title& gui::Title::set_splash(std::string_view text)
{
    m_splash = text;

    return self();
}

gui::Title& gui::Title::set_version(std::string_view text)
{
    m_version = text;

    return self();
}

gui::Title& gui::Title::set_top(float top)
{
    m_top = top;

    return self();
}

gui::Title& gui::Title::set_width(float width)
{
    m_width = width;

    return self();
}

void gui::Title::layout(void)
{
    auto window_size = ImGui::GetWindowSize();
    auto draw_list = ImGui::GetWindowDrawList();

    auto shadow_color = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.850f));

    // The offset follows the UI scale: on a 2x display a single pixel is
    // thinner than one glyph pixel and the shadow reads as a soft edge
    auto shadow_offset = std::max(1.0f, static_cast<float>(globals::gui_scale));

    // In game the menu is a pause screen, so the wordmark and the splash
    // would just cover the world; only the buttons and the version stay
    if(!globals::registry.valid(globals::player)) {
        layout_header(window_size, draw_list, shadow_offset);
    }

    if(!m_version.empty()) {
        ImGui::PushFont(fonts::unscii8, 8.0f);

        auto version_size = ImGui::CalcTextSize(m_version.c_str());
        auto version_pad = VERSION_PAD * static_cast<float>(globals::gui_scale);

        auto version_position = ImVec2();
        version_position.x = version_pad;
        version_position.y = window_size.y - version_size.y - version_pad;

        ImGui::SetCursorPos(version_position);

        auto version_screen = ImGui::GetCursorScreenPos();
        auto version_color = ImGui::GetColorU32(ImVec4(0.850f, 0.850f, 0.850f, 1.000f));

        draw_list->AddText(ImVec2(version_screen.x + shadow_offset, version_screen.y + shadow_offset), shadow_color, m_version.c_str());
        draw_list->AddText(version_screen, version_color, m_version.c_str());

        ImGui::PopFont();
    }
}

void gui::Title::layout_header(ImVec2 window_size, ImDrawList* draw_list, float shadow_offset)
{
    if(m_texture == 0 || m_texture_size.x <= 0.0f || m_texture_size.y <= 0.0f) {
        return;
    }

    auto aspect = m_texture_size.x / m_texture_size.y;

    // The wordmark is very wide, so the height cap is what keeps it
    // from eating the whole screen on wide windows
    auto max_height = 0.225f * window_size.y;
    auto width = std::min(window_size.x * m_width, max_height * aspect);
    auto height = width / aspect;

    auto position = ImVec2(0.5f * (window_size.x - width), window_size.y * m_top);

    ImGui::SetCursorPos(position);
    ImGui::Image(m_texture, ImVec2(width, height));

    if(m_splash.empty()) {
        return;
    }

    auto splash_size = ImGui::CalcTextSize(m_splash.c_str());

    auto splash_position = ImVec2();
    splash_position.x = 0.5f * (window_size.x - splash_size.x);
    splash_position.y = position.y + height + SPLASH_GAP * static_cast<float>(globals::gui_scale);

    ImGui::SetCursorPos(splash_position);

    // Drawn by hand: the sky behind is bright and a flat line of text
    // sinks into it without a shadow
    auto splash_screen = ImGui::GetCursorScreenPos();

    auto shadow_color = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.850f));
    auto text_color = ImGui::GetColorU32(ImVec4(1.000f, 1.000f, 0.333f, 1.000f));

    draw_list->AddText(ImVec2(splash_screen.x + shadow_offset, splash_screen.y + shadow_offset), shadow_color, m_splash.c_str());
    draw_list->AddText(splash_screen, text_color, m_splash.c_str());
}
