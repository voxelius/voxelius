#include "core/pch.hh"

#include "core/cvar/boolean.hh"

#include "core/cvar/binding.hh"

cvar::Boolean::Boolean(bool default_value) : m_value(default_value)
{
    // empty
}

void cvar::Boolean::set_string(std::string_view value)
{
    set_value(string_to_bool(value));
}

std::string_view cvar::Boolean::to_string(void) const
{
    sync_value();

    return bool_to_string(m_value);
}

bool cvar::Boolean::value(void) const
{
    sync_value();

    return m_value;
}

void cvar::Boolean::set_value(bool new_value)
{
    m_value = new_value;

    if(m_binding) {
        if(m_binding->set_value(std::string(bool_to_string(new_value)))) {
            m_generation = m_binding->generation();
        }
        else {
            m_generation = 0;
        }
    }
}

std::string_view cvar::Boolean::bool_to_string(bool value)
{
    if(value) {
        return std::string_view("true");
    }
    else {
        return std::string_view("false");
    }
}

bool cvar::Boolean::string_to_bool(std::string_view value)
{
    auto is_true = false;
    is_true = is_true || 0 == value.compare("true");
    is_true = is_true || 0 == value.compare("yes");
    is_true = is_true || 0 == value.compare("on");
    is_true = is_true || 0 == value.compare("1");

    auto is_false = false;
    is_false = is_false || 0 == value.compare("false");
    is_false = is_false || 0 == value.compare("off");
    is_false = is_false || 0 == value.compare("no");
    is_false = is_false || 0 == value.compare("0");

    return is_true && !is_false;
}

void cvar::Boolean::sync_value(void) const
{
    if(m_binding && m_generation < m_binding->generation()) {
        m_value = string_to_bool(m_binding->value());
        m_generation = m_binding->generation();
    }
}
