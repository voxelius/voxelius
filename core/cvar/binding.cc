#include "core/pch.hh"

#include "core/cvar/binding.hh"

#include "core/cvar/host.hh"
#include "core/msg/log.hh"

cvar::detail::VariableBinding::VariableBinding(std::string name) : m_name(std::move(name))
{
    // empty
}

void cvar::detail::VariableBinding::set_default(std::string default_value)
{
    if(m_default.has_value()) {
        msg::log_warning("{}: default value is already set", m_name);
    }

    if(m_generation == 1 && !set_value(default_value)) {
        msg::log_warning("{}: default value {} is not valid", m_name, default_value);
    }

    m_default = std::move(default_value);
}

void cvar::detail::VariableBinding::set_help(std::string help)
{
    if(m_help.has_value()) {
        msg::log_warning("{}: help is already set", m_name);
    }

    m_help = std::move(help);
}

bool cvar::detail::VariableBinding::set_value(std::string value)
{
    if(!validate(value)) {
        return false;
    }

    if(m_value == value) {
        return true;
    }

    m_value = std::move(value);
    m_generation += 1;

    if(m_flags.value_or(FCVAR_NONE) & FCVAR_REPLICATED) {
        detail::mark_dirty(m_name);
    }

    m_changed.publish();

    return true;
}

void cvar::detail::VariableBinding::set_flags(flag_bits flags)
{
    if(m_flags.has_value()) {
        msg::log_warning("{}: flags are already set", m_name);
    }

    m_flags = flags;
}

bool cvar::detail::VariableBinding::set_shadow(std::string value)
{
    if(!validate(value)) {
        return false;
    }

    if(m_shadow.has_value() && m_shadow.value() == value) {
        return true;
    }

    m_shadow = std::move(value);
    m_generation += 1;

    m_changed.publish();

    return true;
}

void cvar::detail::VariableBinding::clear_shadow(void)
{
    if(m_shadow.has_value()) {
        m_shadow.reset();
        m_generation += 1;

        m_changed.publish();
    }
}

void cvar::detail::VariableBinding::set_remote(bool remote)
{
    m_is_remote = remote;
}

bool cvar::detail::VariableBinding::validate(std::string_view value) const
{
    if(m_validator) {
        return m_validator(value);
    }
    else {
        return true;
    }
}

cvar::detail::variable_sink_type cvar::detail::VariableBinding::changed(void)
{
    return variable_sink_type { m_changed };
}

void cvar::detail::VariableBinding::revert(void)
{
    if(m_default.has_value()) {
        set_value(m_default.value());
    }
}

cvar::detail::CommandBinding::CommandBinding(std::string name) : m_name(std::move(name))
{
    // empty
}

bool cvar::detail::CommandBinding::has_delegate(void) const
{
    return static_cast<bool>(m_delegate);
}

void cvar::detail::CommandBinding::set_help(std::string help)
{
    if(m_help.has_value()) {
        msg::log_warning("{}: help is already set", m_name);
    }

    m_help = std::move(help);
}

void cvar::detail::CommandBinding::set_flags(flag_bits flags)
{
    constexpr static auto variable_flags = FCVAR_ARCHIVE | FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_READONLY;

    if(m_flags.has_value()) {
        msg::log_warning("{}: flags are already set", m_name);
    }

    if(flags & variable_flags) {
        msg::log_warning("{}: stripping variable-only flags", m_name);
        flags &= ~variable_flags;
    }

    m_flags = flags;
}

void cvar::detail::CommandBinding::set_remote(bool remote)
{
    m_is_remote = remote;
}

cvar::status cvar::detail::CommandBinding::invoke(const Context& ctx, std::span<const std::string_view> args) const
{
    if(m_delegate) {
        return m_delegate(ctx, args);
    }
    else {
        return status::UNKNOWN;
    }
}
