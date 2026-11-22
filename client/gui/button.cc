#include "client/pch.hh"

#include "client/gui/button.hh"

#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/style.hh"

gui::Button& gui::Button::set_text(std::string_view key)
{
    m_key = key;

    return self();
}

gui::Button& gui::Button::set_icon(icon_type icon)
{
    m_icon_enabled = true;
    m_icon = icon;

    return self();
}

gui::Button& gui::Button::set_font(ImFont* font, float size)
{
    m_font = font;
    m_font_size = size;

    return self();
}

gui::Button& gui::Button::set_size(float wide, float tall)
{
    m_size = ImVec2(wide, tall);

    return self();
}

gui::Button& gui::Button::on_click(std::function<void(void)> callback)
{
    m_callback = std::move(callback);

    return self();
}

gui::Button& gui::Button::set_enabled(bool enabled)
{
    m_enabled = enabled;

    return self();
}

void gui::Button::layout(void)
{
    auto& palette = style::palette();

    if(m_font) {
        auto font_size = m_font->LegacySize;

        if(m_font_size > 0.0f) {
            font_size = m_font_size;
        }

        ImGui::PushFont(m_font, font_size);
    }

    auto frame_padding = ImGui::GetStyle().FramePadding;
    auto label_size = ImGui::CalcTextSize(m_label.c_str());

    auto default_size = ImVec2();
    default_size.x = label_size.x + 2.0f * frame_padding.x;
    default_size.y = label_size.y + 2.0f * frame_padding.y;

    // Negative sizes mean "fill what is left", the same way ImGui::Button
    // treats them, so the layout code around us keeps working unchanged
    auto size = m_size;

    if(size.x == 0.0f) {
        size.x = default_size.x;
    }
    else if(size.x < 0.0f) {
        size.x = std::max(4.0f, ImGui::GetContentRegionAvail().x + size.x);
    }

    if(size.y == 0.0f) {
        size.y = default_size.y;
    }
    else if(size.y < 0.0f) {
        size.y = std::max(4.0f, ImGui::GetContentRegionAvail().y + size.y);
    }

    ImGui::BeginDisabled(!m_enabled);

    auto clicked = ImGui::InvisibleButton(m_id.c_str(), size);

    auto hovered = ImGui::IsItemHovered();
    auto held = ImGui::IsItemActive();

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();

    auto base = palette.button;

    if(!m_enabled) {
        base = palette.button_disabled;
    }
    else if(held) {
        base = palette.button_active;
    }
    else if(hovered) {
        base = palette.button_hovered;
    }

    auto draw_list = ImGui::GetWindowDrawList();

    detail::draw_stone_face(draw_list, item_min, item_max, base);

    if(m_icon_enabled) {
        auto icon_color = ImGui::GetColorU32(m_enabled ? palette.text : palette.text_disabled);

        detail::draw_icon(draw_list, m_icon, item_min, item_max, icon_color);
    }
    else {
        detail::draw_stone_label(draw_list, item_min, item_max, m_label, held, m_enabled);
    }

    ImGui::EndDisabled();

    if(m_font) {
        ImGui::PopFont();
    }

    if(clicked && m_callback) {
        m_callback();
    }
}

void gui::Button::translate(void)
{
    m_label = language::resolve(m_key);

    m_id = m_label;
    m_id.append(imgui_id());
}
