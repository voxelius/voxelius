#include "core/pch.hh"

#include "core/cvar/number.hh"

#include "core/cvar/binding.hh"

template<vx::arithmetic T>
cvar::Number<T>::Number(T default_value) : m_value(default_value)
{
    m_string = std::to_string(m_value);
}

template<vx::arithmetic T>
cvar::Number<T>::Number(T default_value, T min_value, T max_value) : m_min_value(min_value), m_max_value(max_value)
{
    m_value = std::clamp<T>(default_value, m_min_value, m_max_value);
    m_string = std::to_string(m_value);
}

template<vx::arithmetic T>
void cvar::Number<T>::set_string(std::string_view value)
{
    T new_value;
    auto check = std::from_chars(value.data(), value.data() + value.size(), new_value);

    if(check.ec == std::errc {}) {
        set_value(new_value);
    }
}

template<vx::arithmetic T>
std::string_view cvar::Number<T>::to_string(void) const
{
    sync_value();

    return m_string;
}

template<vx::arithmetic T>
bool cvar::Number<T>::validate(std::string_view value) const
{
    T new_value;
    auto check = std::from_chars(value.data(), value.data() + value.size(), new_value);

    if(check.ec == std::errc()) {
        return new_value >= m_min_value && new_value <= m_max_value;
    }
    else {
        return false;
    }
}

template<vx::arithmetic T>
T cvar::Number<T>::value(void) const
{
    sync_value();

    return m_value;
}

template<vx::arithmetic T>
void cvar::Number<T>::set_value(T new_value)
{
    new_value = std::clamp<T>(new_value, m_min_value, m_max_value);

    m_value = new_value;
    m_string = std::to_string(m_value);

    if(m_binding) {
        if(m_binding->set_value(m_string)) {
            m_generation = m_binding->generation();
        }
        else {
            m_generation = 0;
        }
    }
}

template<vx::arithmetic T>
void cvar::Number<T>::sync_value(void) const
{
    if(m_binding && m_generation < m_binding->generation()) {
        T new_value;
        auto raw = m_binding->value();
        auto check = std::from_chars(raw.data(), raw.data() + raw.size(), new_value);

        if(check.ec == std::errc {}) {
            m_value = std::clamp<T>(new_value, m_min_value, m_max_value);
        }

        m_string = std::to_string(m_value);
        m_generation = m_binding->generation();
    }
}

template class cvar::Number<unsigned char>;
template class cvar::Number<unsigned short>;
template class cvar::Number<unsigned>;
template class cvar::Number<unsigned long>;
template class cvar::Number<unsigned long long>;

template class cvar::Number<signed char>;
template class cvar::Number<short>;
template class cvar::Number<int>;
template class cvar::Number<long>;
template class cvar::Number<long long>;

template class cvar::Number<float>;
template class cvar::Number<double>;
