#include "client/pch.hh"

#include "client/gui/menu.hh"

#include "client/globals.hh"

constexpr static unsigned CONDITION_ANY = 0xFFFF;
constexpr static unsigned CONDITION_INGAME = 0x0001;
constexpr static unsigned CONDITION_OFFLINE = 0x0002;

constexpr static float BUTTON_GAP = 6.0f;

static bool condition_met(unsigned condition)
{
    if(globals::registry.valid(globals::player)) {
        return static_cast<bool>(condition & CONDITION_INGAME);
    }
    else {
        return static_cast<bool>(condition & CONDITION_OFFLINE);
    }
}

gui::Menu& gui::Menu::set_control(ImVec2 control)
{
    m_control = control;

    return self();
}

gui::Menu& gui::Menu::set_anchor(float xrel, float yrel)
{
    m_anchor.x = xrel;
    m_anchor.y = yrel;

    return self();
}

gui::Menu& gui::Menu::add_button_any(std::string_view label, std::function<void(void)> callback)
{
    ButtonItem item {};
    item.condition = CONDITION_ANY;
    item.widget.set_text(label);
    item.widget.on_click(std::move(callback));

    m_items.emplace_back(std::move(item));

    return self();
}

gui::Menu& gui::Menu::add_button_ingame(std::string_view label, std::function<void(void)> callback)
{
    ButtonItem item {};
    item.condition = CONDITION_INGAME;
    item.widget.set_text(label);
    item.widget.on_click(std::move(callback));

    m_items.emplace_back(std::move(item));

    return self();
}

gui::Menu& gui::Menu::add_button_offline(std::string_view label, std::function<void(void)> callback)
{
    ButtonItem item {};
    item.condition = CONDITION_OFFLINE;
    item.widget.set_text(label);
    item.widget.on_click(std::move(callback));

    m_items.emplace_back(std::move(item));

    return self();
}

gui::Menu& gui::Menu::add_spacer_any(float height)
{
    SpacerItem item {};
    item.condition = CONDITION_ANY;
    item.height = height;

    m_items.emplace_back(std::move(item));

    return self();
}

gui::Menu& gui::Menu::add_spacer_ingame(float height)
{
    SpacerItem item {};
    item.condition = CONDITION_INGAME;
    item.height = height;

    m_items.emplace_back(std::move(item));

    return self();
}

gui::Menu& gui::Menu::add_spacer_offline(float height)
{
    SpacerItem item {};
    item.condition = CONDITION_OFFLINE;
    item.height = height;

    m_items.emplace_back(std::move(item));

    return self();
}

void gui::Menu::layout(void)
{
    auto scale = static_cast<float>(globals::gui_scale);

    auto control = ImVec2(m_control.x * scale, m_control.y * scale);
    auto gap = BUTTON_GAP * scale;

    auto window_size = ImGui::GetWindowSize();

    auto cursor = ImVec2();
    cursor.x = window_size.x * m_anchor.x - 0.5f * control.x;
    cursor.y = window_size.y * m_anchor.y;

    for(auto& item : m_items) {
        if(auto button = std::get_if<ButtonItem>(&item)) {
            if(!condition_met(button->condition)) {
                continue;
            }

            ImGui::SetCursorPos(cursor);

            button->widget.set_size(control.x, control.y);
            button->widget.layout();

            cursor.y += control.y + gap;
        }
        else if(auto spacer = std::get_if<SpacerItem>(&item)) {
            if(condition_met(spacer->condition)) {
                cursor.y += spacer->height * scale;
            }
        }
    }
}

void gui::Menu::translate(void)
{
    for(auto& item : m_items) {
        if(auto button = std::get_if<ButtonItem>(&item)) {
            button->widget.translate();
        }
    }
}
