#include "client/pch.hh"

#include "client/gui/video_selector.hh"

#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/video.hh"

gui::VideoSelector& gui::VideoSelector::on_change(std::function<decision_type(const mode_type&)> callback)
{
    m_callback = std::move(callback);

    return self();
}

void gui::VideoSelector::ensure_choices(void)
{
    std::call_once(m_choices_once, [this] {
        m_choices = video::query_fullscreen_modes();

        m_choice_labels.clear();
        m_choice_labels.resize(1 + m_choices.size());

        for(std::size_t i = 0; i < m_choices.size(); ++i) {
            const auto& mode = m_choices[i];

            auto& label = m_choice_labels[i + 1];
            label = std::format("{}x{} @ {}Hz", mode.w, mode.h, static_cast<int>(mode.refresh_rate));
            label += std::format("{}[{}]", imgui_id(), i + 1);
        }
    });
}

void gui::VideoSelector::apply_mode(const mode_type& mode)
{
    if(mode.has_value()) {
        video::request_fullscreen(mode->w, mode->h, static_cast<int>(mode->refresh_rate));
    }
    else {
        video::request_windowed();
    }
}

void gui::VideoSelector::layout_control(void)
{
    ensure_choices();

    if(m_pending) {
        if(m_callback) {
            auto decision = m_callback(m_pending_mode);

            if(decision.has_value()) {
                if(decision.value()) {
                    apply_mode(m_pending_mode);
                }

                m_pending = false;
            }
        }
        else {
            m_pending = false;
        }
    }

    int current_width;
    int current_height;
    int current_rate;
    bool is_fullscreen;

    video::query_current_mode(current_width, current_height, current_rate, is_fullscreen);

    int current_index = 0;

    if(is_fullscreen) {
        for(std::size_t i = 0; i < m_choices.size(); ++i) {
            const auto& mode = m_choices[i];

            if(mode.w == current_width && mode.h == current_height && static_cast<int>(mode.refresh_rate) == current_rate) {
                current_index = static_cast<int>(i) + 1;
                break;
            }
        }
    }

    auto width = ImGui::CalcItemWidth();
    auto height = ImGui::GetFrameHeight();

    auto size = ImVec2(width, height);
    auto& control_id = imgui_id();

    if(gui::detail::stone_combo(control_id.c_str(), m_choice_labels[current_index], size)) {
        ImGui::OpenPopup(control_id.c_str());
    }

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();
    auto open = gui::detail::stone_popup_begin(control_id.c_str(), item_min, item_max);

    if(open) {
        if(ImGui::Selectable(m_choice_labels[0].c_str(), current_index == 0)) {
            if(current_index) {
                m_pending = true;
                m_pending_mode = std::nullopt;
            }
        }

        auto group_width = std::numeric_limits<int>::min();
        auto group_height = std::numeric_limits<int>::min();

        for(std::size_t i = 0; i < m_choices.size(); ++i) {
            const auto& mode = m_choices[i];

            auto choice_index = static_cast<int>(i) + 1;

            if(group_width != mode.w || group_height != mode.h) {
                ImGui::Separator();

                group_width = mode.w;
                group_height = mode.h;
            }

            if(ImGui::Selectable(m_choice_labels[choice_index].c_str(), current_index == choice_index)) {
                if(current_index != choice_index) {
                    m_pending = true;
                    m_pending_mode = mode;
                }
            }
        }
    }

    gui::detail::stone_popup_end(open);
}

void gui::VideoSelector::translate_control(void)
{
    ensure_choices();

    auto label = language::resolve("settings.choice.video_mode.windowed");
    auto id = imgui_id();

    m_choice_labels[0] = std::format("{}{}[0]", label, id);
}
