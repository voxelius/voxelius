#include "client/pch.hh"

#include "client/gui/command_line.hh"

#include "client/gui/stone.hh"
#include "client/language.hh"

gui::CommandLine& gui::CommandLine::set_font(ImFont* font, float size)
{
    m_font = font;
    m_font_size = size;

    return self();
}

gui::CommandLine& gui::CommandLine::set_button_text(std::string_view key)
{
    m_button_key = key;

    return self();
}

gui::CommandLine& gui::CommandLine::set_history_size(std::size_t size)
{
    m_history_size = std::max<std::size_t>(size, 1);

    while(m_history.size() > m_history_size) {
        m_history.pop_front();
    }

    return self();
}

gui::CommandLine& gui::CommandLine::on_submit(std::function<void(std::string_view)> callback)
{
    m_submit = std::move(callback);

    return self();
}

void gui::CommandLine::focus(void)
{
    m_focus = true;
    m_swallow = true;
}

void gui::CommandLine::clear(void)
{
    m_buffer.clear();
    m_history_pos = -1;
}

int gui::CommandLine::on_callback(ImGuiInputTextCallbackData* data)
{
    auto widget = reinterpret_cast<CommandLine*>(data->UserData);

    if(data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        return widget->m_swallow ? 1 : 0;
    }

    if(data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        auto previous_pos = widget->m_history_pos;
        auto history_size = static_cast<int>(widget->m_history.size());

        if(data->EventKey == ImGuiKey_UpArrow) {
            if(widget->m_history_pos < 0) {
                widget->m_history_pos = history_size - 1;
            }
            else if(widget->m_history_pos > 0) {
                widget->m_history_pos -= 1;
            }
        }
        else if(data->EventKey == ImGuiKey_DownArrow) {
            if(widget->m_history_pos >= 0) {
                widget->m_history_pos += 1;

                if(widget->m_history_pos >= history_size) {
                    widget->m_history_pos = -1;
                }
            }
        }

        if(previous_pos != widget->m_history_pos) {
            std::string entry {};

            if(widget->m_history_pos >= 0) {
                entry = widget->m_history[static_cast<std::size_t>(widget->m_history_pos)];
            }

            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, entry.c_str());
        }
    }

    return 0;
}

void gui::CommandLine::submit(void)
{
    if(m_buffer.empty()) {
        return;
    }

    if(m_history.empty() || m_history.back() != m_buffer) {
        m_history.emplace_back(m_buffer);

        while(m_history.size() > m_history_size) {
            m_history.pop_front();
        }
    }

    if(m_submit) {
        m_submit(m_buffer);
    }

    m_buffer.clear();
    m_history_pos = -1;
    m_focus = true;
}

void gui::CommandLine::layout(void)
{
    if(m_font) {
        auto size = m_font->LegacySize;

        if(m_font_size > 0.0f) {
            size = m_font_size;
        }

        ImGui::PushFont(m_font, size);
    }

    const auto& style = ImGui::GetStyle();

    auto button_width = 0.0f;

    if(m_button_label.size()) {
        button_width = ImGui::CalcTextSize(m_button_text.c_str()).x + 2.0f * style.FramePadding.x + style.ItemSpacing.x;
    }

    if(m_focus) {
        ImGui::SetKeyboardFocusHere();

        m_focus = false;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - button_width);

    ImGuiInputTextFlags input_flags = 0;
    input_flags |= ImGuiInputTextFlags_EnterReturnsTrue;
    input_flags |= ImGuiInputTextFlags_CallbackCharFilter;
    input_flags |= ImGuiInputTextFlags_CallbackHistory;

    auto submitted = ImGui::InputText(imgui_id().c_str(), &m_buffer, input_flags, &CommandLine::on_callback, this);
    auto input_active = ImGui::IsItemActive();

    if(m_button_label.size()) {
        ImGui::SameLine();

        auto avail = ImGui::GetContentRegionAvail();
        auto height = ImGui::GetFrameHeight();

        auto button_size = ImVec2(avail.x, height);

        if(gui::detail::stone_button(m_button_label.c_str(), m_button_label, button_size)) {
            submitted = true;
        }
    }

    if(submitted) {
        submit();
    }

    if(m_swallow && input_active) {
        m_swallow = false;
    }

    if(!input_active && !ImGui::IsAnyItemActive()) {
        m_focus = true;
    }

    if(m_font) {
        ImGui::PopFont();
    }
}

void gui::CommandLine::translate(void)
{
    if(m_button_key.empty()) {
        m_button_text.clear();
        m_button_label.clear();
        return;
    }

    m_button_text = language::resolve(m_button_key);
    m_button_label = std::format("{}{}[button]", m_button_text, imgui_id());
}
