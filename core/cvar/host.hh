#ifndef E0FEE24F_0FAF_486E_BB2D_3CD62EA209A8
#define E0FEE24F_0FAF_486E_BB2D_3CD62EA209A8

#include "core/cvar/binding.hh"
#include "core/cvar/status.hh"

namespace cvar
{
class Variable;
} // namespace cvar

namespace cvar
{
using write_hook_type = entt::delegate<void(std::ostream& stream)>;
using remote_exec_type = entt::delegate<status(std::string_view line)>;
} // namespace cvar

namespace cvar::detail
{
void add_write_hook(write_hook_type hook);
} // namespace cvar::detail

namespace cvar::detail
{
void mark_dirty(std::string_view name);
VariableBinding& make_remote_var(std::string_view name);
CommandBinding& make_remote_cmd(std::string_view name);
} // namespace cvar::detail

namespace cvar
{
void set_remote_exec(remote_exec_type hook);
} // namespace cvar

namespace cvar
{
std::span<const std::string> take_dirty(void);
void clear_dirty(void);
void clear_remote(void);
} // namespace cvar

namespace cvar
{
detail::VariableBinding& bind_var(Variable& variable, std::string_view name);
detail::CommandBinding& bind_cmd(std::string_view name);
} // namespace cvar

namespace cvar
{
detail::VariableBinding* find_var(std::string_view name);
detail::CommandBinding* find_cmd(std::string_view name);
} // namespace cvar

namespace cvar
{
status exec(std::string_view line, const Context& ctx = {});
status exec(std::string_view name, std::span<const std::string_view> args, const Context& ctx = {});
} // namespace cvar

namespace cvar
{
bool exec_from(std::string_view path);

// A variable flagged with any bit in ignore_flags stays out of the file;
// the client uses it to drop server-owned variables from client.conf
void save_values(std::string_view path, flag_bits ignore_flags = FCVAR_NONE);
} // namespace cvar

namespace cvar
{
template<auto Candidate>
void add_write_hook(void);
} // namespace cvar

namespace cvar
{
void for_each_var(flag_bits mask, std::function<void(const detail::VariableBinding&)> callback);
void for_each_cmd(flag_bits mask, std::function<void(const detail::CommandBinding&)> callback);
} // namespace cvar

template<auto Candidate>
void cvar::add_write_hook(void)
{
    write_hook_type hook {};
    hook.connect<Candidate>();

    detail::add_write_hook(std::move(hook));
}

#endif /* E0FEE24F_0FAF_486E_BB2D_3CD62EA209A8 */
