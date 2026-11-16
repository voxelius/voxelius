#include "client/pch.hh"

#include "client/gui/language_selector.hh"

#include "client/gui/stone.hh"
#include "client/language.hh"

void gui::LanguageSelector::layout_control(void)
{
    auto current = language::current();
    std::string current_label(current->endonym());

    auto size = ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeight());
    auto& control_id = imgui_id();

    if(gui::detail::stone_combo(control_id.c_str(), current_label, size)) {
        ImGui::OpenPopup(control_id.c_str());
    }

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();
    auto open = gui::detail::stone_popup_begin(control_id.c_str(), item_min, item_max);

    if(open) {
        auto it = language::const_begin();

        while(it < language::const_end()) {
            if(ImGui::Selectable(it->display_cstr(), it == current)) {
                language::set(it);
            }

            it = std::next(it);
        }
    }

    gui::detail::stone_popup_end(open);
}

void gui::LanguageSelector::translate_control(void)
{
    // empty
}
