#include "client/pch.hh"

#include "client/gui/checkbox.hh"

#include "core/cvar/host.hh"

#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/style.hh"

gui::CheckBox& gui::CheckBox::bind(std::string_view name)
{
    set_key(name);

    cvar::bind_var(m_value, name);

    return self();
}

bool gui::CheckBox::value(void) const
{
    return m_value.value();
}

void gui::CheckBox::set_value(bool value)
{
    m_value.set_value(value);
}

void gui::CheckBox::layout_control(void)
{
    auto& palette = style::palette();

    auto current = m_value.value();

    auto width = ImGui::CalcItemWidth();
    auto height = ImGui::GetFrameHeight();

    auto size = ImVec2(width, height);

    auto clicked = ImGui::InvisibleButton(imgui_id().c_str(), size);

    auto hovered = ImGui::IsItemHovered();
    auto held = ImGui::IsItemActive();

    auto base = current ? palette.button_active : palette.button;

    if(held) {
        base = palette.button_active;
    }
    else if(hovered && !current) {
        base = palette.button_hovered;
    }

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();

    auto draw_list = ImGui::GetWindowDrawList();

    detail::draw_stone_face(draw_list, item_min, item_max, base);
    detail::draw_stone_label(draw_list, item_min, item_max, current ? m_label_true : m_label_false, held, true);

    if(clicked) {
        m_value.set_value(!current);
    }
}

void gui::CheckBox::translate_control(void)
{
    m_label_false = language::resolve("settings.checkbox.false");
    m_label_true = language::resolve("settings.checkbox.true");
}
