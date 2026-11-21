#include "core/pch.hh"

#include "core/cvar/variable.hh"

#include "core/cvar/binding.hh"

cvar::Variable::~Variable(void)
{
    if(m_binding) {
        detach();
    }
}

bool cvar::Variable::validate(std::string_view) const
{
    return true;
}

void cvar::Variable::attach(std::shared_ptr<detail::VariableBinding>& binding)
{
    if(m_binding) {
        detach();
    }

    m_binding = binding;
    m_generation = 0;

    binding->m_variables.push_back(this);
}

void cvar::Variable::detach(void)
{
    if(m_binding) {
        std::erase(m_binding->m_variables, this);

        if(m_binding->m_validator.data() == this) {
            // The binding was validated through this variable's
            // virtual validate; the delegate must not outlive it
            m_binding->m_validator.reset();
        }

        m_binding = nullptr;
        m_generation = 0;
    }
}
