#include "client/pch.hh"

#include "client/gui/input.hh"

#include "core/cvar/host.hh"

template<typename T>
gui::InputSetting<T>& gui::InputSetting<T>::bind(std::string_view name)
{
    this->set_key(name);

    cvar::bind_var(m_value, name);

    return this->self();
}

template<typename T>
gui::InputSetting<T>& gui::InputSetting<T>::set_range(T min, T max)
{
    m_min = min;
    m_max = max;

    return this->self();
}

template<typename T>
T gui::InputSetting<T>::value(void) const
{
    return m_value.value();
}

template<typename T>
void gui::InputSetting<T>::set_value(T value)
{
    m_value.set_value(value);
}

template<>
void gui::InputSetting<int>::layout_control(void)
{
    auto current = m_value.value();

    if(ImGui::InputInt(this->imgui_id().c_str(), &current)) {
        m_value.set_value(std::clamp(current, m_min, m_max));
    }
}

template<>
void gui::InputSetting<float>::layout_control(void)
{
    auto current = m_value.value();

    if(ImGui::InputFloat(this->imgui_id().c_str(), &current, 0.0f, 0.0f, "%.3f")) {
        m_value.set_value(std::clamp(current, m_min, m_max));
    }
}

template<>
void gui::InputSetting<unsigned>::layout_control(void)
{
    auto current = m_value.value();
    auto scalar = static_cast<std::uint32_t>(current);

    if(ImGui::InputScalar(this->imgui_id().c_str(), ImGuiDataType_U32, &scalar)) {
        m_value.set_value(std::clamp(static_cast<unsigned>(scalar), m_min, m_max));
    }
}

template<typename T>
void gui::InputSetting<T>::translate_control(void)
{
    // empty
}

template class gui::InputSetting<int>;
template class gui::InputSetting<float>;
template class gui::InputSetting<unsigned>;
