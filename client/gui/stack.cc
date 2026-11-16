#include "client/pch.hh"

#include "client/gui/stack.hh"

#include "client/globals.hh"

gui::HorizontalStack& gui::HorizontalStack::add_item(Widget& widget, stack_policy_type policy, float fixed_width)
{
    Item item {};
    item.widget = &widget;
    item.policy = policy;
    item.fixed_width = fixed_width;

    m_items.emplace_back(std::move(item));

    return self();
}

void gui::HorizontalStack::layout(void)
{
    auto& style = ImGui::GetStyle();
    auto avail = ImGui::GetContentRegionAvail();

    auto fixed_total = 0.0f;
    auto expanding_count = 0U;

    for(const auto& item : m_items) {
        if(item.policy == FIXED) {
            fixed_total += item.fixed_width * static_cast<float>(globals::gui_scale);
        }
        else {
            expanding_count += 1;
        }
    }

    auto spacing_total = 0.0f;
    auto expanding_width = 0.0f;

    if(m_items.size()) {
        spacing_total = style.ItemSpacing.x * static_cast<float>(m_items.size() - 1);
    }

    if(expanding_count) {
        expanding_width = avail.x - fixed_total - spacing_total;
        expanding_width /= static_cast<float>(expanding_count);
    }

    expanding_width = std::max(expanding_width, 1.0f);

    for(std::size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];

        auto width = expanding_width;

        if(item.policy == FIXED) {
            width = item.fixed_width * static_cast<float>(globals::gui_scale);
        }

        ImGui::PushID(static_cast<int>(i));

        if(ImGui::BeginChild("###item", ImVec2(width, avail.y))) {
            if(item.widget->visible()) {
                item.widget->layout();
            }
        }

        ImGui::EndChild();
        ImGui::PopID();

        if(i + 1 < m_items.size()) {
            ImGui::SameLine();
        }
    }
}

void gui::HorizontalStack::translate(void)
{
    for(auto& item : m_items) {
        item.widget->translate();
    }
}

gui::VerticalStack& gui::VerticalStack::add_item(Widget& widget, stack_policy_type policy, float fixed_height)
{
    Item item {};
    item.widget = &widget;
    item.policy = policy;
    item.fixed_height = fixed_height;

    m_items.emplace_back(std::move(item));

    return self();
}

void gui::VerticalStack::layout(void)
{
    auto& style = ImGui::GetStyle();
    auto avail = ImGui::GetContentRegionAvail();

    auto fixed_total = 0.0f;
    auto expanding_count = 0U;

    for(const auto& item : m_items) {
        if(item.policy == FIXED) {
            fixed_total += item.fixed_height * static_cast<float>(globals::gui_scale);
        }
        else {
            expanding_count += 1;
        }
    }

    auto spacing_total = 0.0f;
    auto expanding_height = 0.0f;

    if(m_items.size()) {
        spacing_total = style.ItemSpacing.y * static_cast<float>(m_items.size() - 1);
    }

    if(expanding_count) {
        expanding_height = avail.y - fixed_total - spacing_total;
        expanding_height /= static_cast<float>(expanding_count);
    }

    expanding_height = std::max(expanding_height, 1.0f);

    // A child item resets the cursor to the window edge, so every row is
    // pinned back to the column the stack started at
    auto origin = ImGui::GetCursorScreenPos();
    auto cursor_y = origin.y;

    for(std::size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];

        auto height = expanding_height;

        if(item.policy == FIXED) {
            height = item.fixed_height * static_cast<float>(globals::gui_scale);
        }

        ImGui::SetCursorScreenPos(ImVec2(origin.x, cursor_y));

        ImGui::PushID(static_cast<int>(i));

        if(ImGui::BeginChild("###item", ImVec2(avail.x, height))) {
            if(item.widget->visible()) {
                item.widget->layout();
            }
        }

        ImGui::EndChild();
        ImGui::PopID();

        cursor_y += height + style.ItemSpacing.y;
    }
}

void gui::VerticalStack::translate(void)
{
    for(auto& item : m_items) {
        item.widget->translate();
    }
}
