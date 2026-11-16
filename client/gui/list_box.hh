#ifndef C54F2663_7A25_4729_AEA3_64ED51F441F0
#define C54F2663_7A25_4729_AEA3_64ED51F441F0

#include "client/globals.hh"

#include "client/gui/container.hh"

namespace gui
{
class ListBox final : public ContainerBuilder<ListBox> {
public:
    ListBox& set_size(ImVec2 size);

    virtual void layout(void) override;

private:
    ImVec2 m_size { -1.0f, -1.0f };
};
} // namespace gui

namespace gui
{
template<typename Derived>
class ListBoxItemBuilder : public WidgetBuilder<Derived> {
public:
    Derived& set_selected(bool selected);
    Derived& on_single_click(std::function<void(void)> callback);
    Derived& on_double_click(std::function<void(void)> callback);

protected:
    ImVec2 begin_row(const ImVec2& row_size);

private:
    bool m_selected { false };
    std::function<void(void)> m_on_single_click {};
    std::function<void(void)> m_on_double_click {};
};
} // namespace gui

template<typename Derived>
Derived& gui::ListBoxItemBuilder<Derived>::set_selected(bool selected)
{
    m_selected = selected;

    return this->self();
}

template<typename Derived>
Derived& gui::ListBoxItemBuilder<Derived>::on_single_click(std::function<void(void)> callback)
{
    m_on_single_click = std::move(callback);

    return this->self();
}

template<typename Derived>
Derived& gui::ListBoxItemBuilder<Derived>::on_double_click(std::function<void(void)> callback)
{
    m_on_double_click = std::move(callback);

    return this->self();
}

template<typename Derived>
ImVec2 gui::ListBoxItemBuilder<Derived>::begin_row(const ImVec2& row_size)
{
    auto cursor = ImGui::GetCursorScreenPos();
    auto avail = ImGui::GetContentRegionAvail();
    auto width = avail.x;

    if(row_size.x > 0.0f) {
        width = row_size.x;
    }

    auto& control_id = this->imgui_id();

    if(ImGui::Selectable(control_id.c_str(), m_selected, 0, row_size)) {
        if(m_on_single_click) {
            m_on_single_click();
        }
    }

    if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if(m_on_double_click) {
            m_on_double_click();
        }
    }

    if(m_selected) {
        auto draw_list = ImGui::GetWindowDrawList();

        ImVec2 end;
        end.x = cursor.x + width;
        end.y = cursor.y + row_size.y;

        // A hairline outline disappears next to a scaled row, so it is one
        // interface pixel thick like the other stone details
        auto thickness = static_cast<float>(globals::gui_scale);

        draw_list->AddRect(cursor, end, ImGui::GetColorU32(ImGuiCol_Text), 0.0f, 0, thickness);
    }

    return cursor;
}

#endif /* C54F2663_7A25_4729_AEA3_64ED51F441F0 */
