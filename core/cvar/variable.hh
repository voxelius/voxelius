#ifndef EDFEC542_68AD_4B5C_A041_30FCCEF254DE
#define EDFEC542_68AD_4B5C_A041_30FCCEF254DE

#include "core/cvar/bits.hh"

namespace cvar::detail
{
class VariableBinding;
} // namespace cvar::detail

namespace cvar
{
class Variable;
} // namespace cvar

namespace cvar
{
detail::VariableBinding& bind_var(Variable& variable, std::string_view name);
} // namespace cvar

namespace cvar
{
class Variable {
public:
    Variable(const Variable& other) = delete;
    Variable& operator=(const Variable& other) = delete;

    Variable(Variable&& other) = delete;
    Variable& operator=(Variable&& other) = delete;

    virtual ~Variable(void);
    virtual void set_string(std::string_view value) = 0;
    virtual std::string_view to_string(void) const = 0;

    virtual bool validate(std::string_view value) const;

protected:
    constexpr Variable(void) = default;

    void attach(std::shared_ptr<detail::VariableBinding>& binding);
    void detach(void);

    friend detail::VariableBinding& cvar::bind_var(Variable& variable, std::string_view name);

    mutable std::uint64_t m_generation { 0 };
    std::shared_ptr<detail::VariableBinding> m_binding { nullptr };
};
} // namespace cvar

#endif /* EDFEC542_68AD_4B5C_A041_30FCCEF254DE */
