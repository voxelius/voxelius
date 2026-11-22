#include "client/pch.hh"

#include "client/gui/rail.hh"

#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/style.hh"

constexpr static float ITEM_HEIGHT = 20.0f;
constexpr static float ITEM_GAP = 4.0f;
constexpr static float CONTENT_GAP = 8.0f;
constexpr static float LABEL_PAD = 6.0f;

static void draw_item(ImDrawList* draw_list, ImVec2 item_min, ImVec2 item_max, std::string_view label, bool selected, bool hovered)
{
    auto& palette = style::palette();

    if(selected) {
        draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.button_active));
        draw_list->AddRect(item_min, item_max, ImGui::GetColorU32(palette.bevel_light));
    }
    else if(hovered) {
        draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.button));
    }

    auto pad = LABEL_PAD * static_cast<float>(globals::gui_scale);
    auto label_size = ImGui::CalcTextSize(label.data(), label.data() + label.size());

    auto label_position = ImVec2();
    label_position.x = item_min.x + pad;
    label_position.y = std::floor(item_min.y + 0.5f * (item_max.y - item_min.y - label_size.y));

    auto text_color = ImGui::GetColorU32(palette.text);
    auto shadow_color = ImGui::GetColorU32(palette.bevel_dark);

    auto shadow_offset = gui::detail::text_shadow_offset();

    draw_list->AddText(ImVec2(label_position.x + shadow_offset, label_position.y + shadow_offset), shadow_color, label.data(),
        label.data() + label.size());
    draw_list->AddText(label_position, text_color, label.data(), label.data() + label.size());
}

gui::Rail& gui::Rail::set_width(float width)
{
    m_width = width;

    return self();
}

gui::Rail& gui::Rail::set_callback(std::function<void(void)> callback)
{
    m_callback = std::move(callback);
    m_callback_label = "<<";

    return self();
}

gui::Rail& gui::Rail::add_page(std::string_view label_key, Widget& content, int priority)
{
    Page page {};
    page.label_key = label_key;
    page.content = &content;
    page.priority = priority;

    auto it = std::ranges::upper_bound(m_pages, priority, std::less {}, &Page::priority);
    m_pages.insert(it, std::move(page));

    return self();
}

void gui::Rail::select(std::size_t index)
{
    m_selected = index;
}

void gui::Rail::layout(void)
{
    auto scale = static_cast<float>(globals::gui_scale);

    auto rail_width = m_width * scale;
    auto item_height = ITEM_HEIGHT * scale;
    auto item_gap = ITEM_GAP * scale;
    auto content_gap = CONTENT_GAP * scale;

    auto origin = ImGui::GetCursorScreenPos();
    auto avail = ImGui::GetContentRegionAvail();

    auto draw_list = ImGui::GetWindowDrawList();

    for(std::size_t i = 0; i < m_pages.size(); ++i) {
        auto item_min = ImVec2(origin.x, origin.y + static_cast<float>(i) * (item_height + item_gap));
        auto item_max = ImVec2(item_min.x + rail_width, item_min.y + item_height);

        ImGui::PushID(static_cast<int>(i));
        ImGui::SetCursorScreenPos(item_min);

        if(ImGui::InvisibleButton("item", ImVec2(rail_width, item_height))) {
            select(i);
        }

        auto hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        draw_item(draw_list, item_min, item_max, m_pages[i].label, i == m_selected, hovered);
    }

    if(m_callback) {
        auto item_min = ImVec2(origin.x, origin.y + avail.y - item_height);
        auto item_max = ImVec2(item_min.x + rail_width, item_min.y + item_height);

        ImGui::SetCursorScreenPos(item_min);

        if(ImGui::InvisibleButton("##back", ImVec2(rail_width, item_height))) {
            m_callback();
        }

        draw_item(draw_list, item_min, item_max, m_callback_label, false, ImGui::IsItemHovered());
    }

    auto divider_x = origin.x + rail_width + 0.5f * content_gap;

    auto& palette = style::palette();
    auto divider_color = ImVec4(palette.bevel_light.x, palette.bevel_light.y, palette.bevel_light.z, 0.35f);

    draw_list->AddLine(ImVec2(divider_x, origin.y), ImVec2(divider_x, origin.y + avail.y), ImGui::GetColorU32(divider_color));

    ImGui::SetCursorScreenPos(ImVec2(origin.x + rail_width + content_gap, origin.y));

    if(!m_pages.empty() && m_pages[m_selected].content->visible()) {
        m_pages[m_selected].content->layout();
    }
}

void gui::Rail::translate(void)
{
    for(auto& page : m_pages) {
        page.label = language::resolve(page.label_key);
        page.content->translate();
    }
}
