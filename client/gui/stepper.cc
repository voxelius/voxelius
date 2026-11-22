#include "client/pch.hh"

#include "client/gui/stepper.hh"

#include "core/cvar/host.hh"

#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/style.hh"

template<typename T>
gui::Stepper<T>& gui::Stepper<T>::bind(std::string_view name)
{
    this->set_key(name);

    cvar::bind_var(m_value, name);

    return this->self();
}

template<typename T>
gui::Stepper<T>& gui::Stepper<T>::set_range(T min, T max, T step)
{
    m_min = min;
    m_max = max;
    m_step = step;

    return this->self();
}

template<typename T>
T gui::Stepper<T>::value(void) const
{
    return m_value.value();
}

template<typename T>
void gui::Stepper<T>::set_value(T value)
{
    m_value.set_value(value);
}

template<typename T>
void gui::Stepper<T>::layout_control(void)
{
    if(m_labels.empty()) {
        return;
    }

    auto& palette = style::palette();

    auto current = m_value.value();
    auto index = static_cast<std::size_t>((current - m_min) / m_step);

    auto scale = static_cast<float>(globals::gui_scale);
    auto bevel = std::max(1.0f, scale);

    auto frame_height = ImGui::GetFrameHeight();
    auto arrow_width = frame_height;
    auto width = std::max(3.0f * arrow_width, ImGui::CalcItemWidth());
    auto middle_width = width - 2.0f * arrow_width;

    auto item_min = ImGui::GetCursorScreenPos();
    auto item_max = ImVec2(item_min.x + width, item_min.y + frame_height);

    auto draw_list = ImGui::GetWindowDrawList();

    // One inset trough holds both arrows and the value,
    // instead of three separate widgets
    draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.frame));
    draw_list->AddRect(item_min, item_max, ImGui::GetColorU32(palette.border));

    draw_list->AddRectFilled(item_min, ImVec2(item_max.x, item_min.y + bevel), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(item_min, ImVec2(item_min.x + bevel, item_max.y), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(ImVec2(item_min.x, item_max.y - bevel), item_max, ImGui::GetColorU32(palette.bevel_light));
    draw_list->AddRectFilled(ImVec2(item_max.x - bevel, item_min.y), item_max, ImGui::GetColorU32(palette.bevel_light));

    auto previous_min = item_min;
    auto previous_max = ImVec2(item_min.x + arrow_width, item_max.y);

    auto value_min = ImVec2(item_min.x + arrow_width, item_min.y);
    auto value_max = ImVec2(value_min.x + middle_width, item_max.y);

    auto next_min = ImVec2(value_max.x, item_min.y);
    auto next_max = item_max;

    auto& control_id = this->imgui_id();

    ImGui::PushID(control_id.c_str());

    ImGui::SetCursorScreenPos(previous_min);
    auto previous_clicked = ImGui::InvisibleButton("###prev", ImVec2(arrow_width, frame_height));

    ImGui::SetCursorScreenPos(value_min);
    auto value_clicked = ImGui::InvisibleButton("###value", ImVec2(middle_width, frame_height));

    ImGui::SetCursorScreenPos(next_min);
    auto next_clicked = ImGui::InvisibleButton("###next", ImVec2(arrow_width, frame_height));

    if(previous_clicked) {
        current = (current <= m_min) ? m_max : static_cast<T>(current - m_step);
        m_value.set_value(current);
    }
    else if(next_clicked) {
        current = (current >= m_max) ? m_min : static_cast<T>(current + m_step);
        m_value.set_value(current);
    }

    index = std::min(static_cast<std::size_t>((current - m_min) / m_step), m_labels.size() - 1);

    detail::draw_stone_label(draw_list, previous_min, previous_max, "<", false, true);
    detail::draw_stone_label(draw_list, next_min, next_max, ">", false, true);
    detail::draw_stone_label(draw_list, value_min, value_max, m_labels[index], false, true);

    if(value_clicked) {
        ImGui::OpenPopup("###list");
    }

    auto open = detail::stone_popup_begin("###list", value_min, value_max);

    if(open) {
        for(std::size_t i = 0; i < m_labels.size(); ++i) {
            if(ImGui::Selectable(m_labels[i].c_str(), i == index)) {
                m_value.set_value(static_cast<T>(m_min + static_cast<T>(i) * m_step));
            }
        }
    }

    detail::stone_popup_end(open);

    ImGui::PopID();
}

template<typename T>
void gui::Stepper<T>::translate_control(void)
{
    auto count = static_cast<std::size_t>((m_max - m_min) / m_step) + 1;

    m_labels.resize(count);

    for(std::size_t i = 0; i < count; ++i) {
        m_labels[i] = language::resolve(std::format("settings.value.{}.{}", this->m_key, i));
    }
}

template class gui::Stepper<int>;
template class gui::Stepper<unsigned>;
