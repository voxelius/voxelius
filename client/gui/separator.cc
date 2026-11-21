#include "client/pch.hh"

#include "client/gui/separator.hh"

#include "client/language.hh"

gui::Separator& gui::Separator::set_text(std::string_view key)
{
    m_key = key;

    return self();
}

void gui::Separator::layout(void)
{
    if(m_text.empty()) {
        ImGui::Separator();
    }
    else {
        ImGui::SeparatorText(m_text.c_str());
    }
}

void gui::Separator::translate(void)
{
    if(m_key.empty()) {
        m_text.clear();
    }
    else {
        m_text = language::resolve(m_key);
    }
}
