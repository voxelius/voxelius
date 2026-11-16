#include "client/pch.hh"

#include "client/gui/menu_button.hh"

#include "client/language.hh"
#include "client/style.hh"

gui::MenuButton& gui::MenuButton::set_icon(icon_type icon)
{
    m_icon = icon;

    return self();
}

gui::MenuButton& gui::MenuButton::add_item(std::string_view key, std::function<void(void)> callback)
{
    Item item {};
    item.key = key;
    item.callback = std::move(callback);

    m_items.emplace_back(std::move(item));

    return self();
}

void gui::MenuButton::layout(void)
{
    auto& palette = style::palette();

    auto size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight());

    auto& control_id = imgui_id();

    auto clicked = ImGui::InvisibleButton(control_id.c_str(), size);

    auto hovered = ImGui::IsItemHovered();
    auto held = ImGui::IsItemActive();

    auto base = palette.button;

    if(held) {
        base = palette.button_active;
    }
    else if(hovered) {
        base = palette.button_hovered;
    }

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();

    auto draw_list = ImGui::GetWindowDrawList();

    gui::detail::draw_stone_face(draw_list, item_min, item_max, base);
    gui::detail::draw_icon(draw_list, m_icon, item_min, item_max, ImGui::GetColorU32(palette.text));

    if(clicked) {
        ImGui::OpenPopup(control_id.c_str());
    }

    auto open = gui::detail::stone_popup_begin(control_id.c_str(), item_min, item_max);

    if(open) {
        for(auto& item : m_items) {
            if(ImGui::Selectable(item.label.c_str())) {
                if(item.callback) {
                    item.callback();
                }

                ImGui::CloseCurrentPopup();
            }
        }
    }

    gui::detail::stone_popup_end(open);
}

void gui::MenuButton::translate(void)
{
    for(auto& item : m_items) {
        item.label = language::resolve(item.key);
    }
}
