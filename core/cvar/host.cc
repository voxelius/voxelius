#include "core/pch.hh"

#include "core/cvar/host.hh"

#include "core/cvar/binding.hh"
#include "core/cvar/variable.hh"
#include "core/msg/log.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"

static vx::hash_map<std::string, std::shared_ptr<cvar::detail::VariableBinding>> s_variables {};
static vx::hash_map<std::string, std::shared_ptr<cvar::detail::CommandBinding>> s_commands {};
static std::vector<cvar::write_hook_type> s_write_hooks {};

static vx::hash_set<std::string> s_dirty_set {};
static std::vector<std::string> s_dirty_list {};
static cvar::remote_exec_type s_remote_exec {};

void cvar::detail::add_write_hook(write_hook_type hook)
{
    assert(static_cast<bool>(hook));

    s_write_hooks.emplace_back(std::move(hook));
}

static std::string rebuild_line(std::string_view name, std::span<const std::string_view> args)
{
    std::string line(name);

    for(auto arg : args) {
        line.push_back(' ');

        if(arg.empty() || utils::has_whitespace(arg)) {
            line.append(std::format("\"{}\"", utils::escape_quotations(arg)));
        }
        else {
            line.append(arg);
        }
    }

    return line;
}

// A context without a client is local: the server
// config, the server itself, or the client's own console.
static bool is_trusted(const cvar::Context& ctx)
{
    return !ctx.client_id.has_value() || ctx.is_operator;
}

void cvar::detail::mark_dirty(std::string_view name)
{
    s_dirty_set.emplace(std::string(name));
}

cvar::detail::VariableBinding& cvar::detail::make_remote_var(std::string_view name)
{
    auto it = s_variables.find(name);

    if(it == s_variables.cend()) {
        auto new_binding = std::make_shared<VariableBinding>(std::string(name));
        auto [inserted_it, inserted] = s_variables.try_emplace(std::string(name), std::move(new_binding));
        inserted_it->second->set_remote(true);
        return *inserted_it->second;
    }
    else {
        return *it->second;
    }
}

cvar::detail::CommandBinding& cvar::detail::make_remote_cmd(std::string_view name)
{
    auto it = s_commands.find(name);

    if(it == s_commands.cend()) {
        auto new_binding = std::make_shared<CommandBinding>(std::string(name));
        auto [inserted_it, inserted] = s_commands.try_emplace(std::string(name), std::move(new_binding));
        inserted_it->second->set_remote(true);
        return *inserted_it->second;
    }
    else {
        it->second->set_remote(true);
        return *it->second;
    }
}

void cvar::set_remote_exec(remote_exec_type hook)
{
    s_remote_exec = std::move(hook);
}

std::span<const std::string> cvar::take_dirty(void)
{
    s_dirty_list.assign(s_dirty_set.cbegin(), s_dirty_set.cend());
    s_dirty_set.clear();
    return s_dirty_list;
}

void cvar::clear_dirty(void)
{
    s_dirty_set.clear();
    s_dirty_list.clear();
}

void cvar::clear_remote(void)
{
    auto var_it = s_variables.begin();

    while(var_it != s_variables.end()) {
        if(var_it->second->is_remote() && !var_it->second->is_bound()) {
            var_it = s_variables.erase(var_it);
        }
        else {
            var_it->second->clear_shadow();
            var_it->second->set_remote(false);
            var_it = std::next(var_it);
        }
    }

    auto cmd_it = s_commands.begin();

    while(cmd_it != s_commands.end()) {
        if(cmd_it->second->is_remote() && !cmd_it->second->has_delegate()) {
            cmd_it = s_commands.erase(cmd_it);
        }
        else {
            cmd_it->second->set_remote(false);
            cmd_it = std::next(cmd_it);
        }
    }
}

cvar::detail::VariableBinding& cvar::bind_var(Variable& variable, std::string_view name)
{
    auto it = s_variables.find(name);

    if(it == s_variables.cend()) {
        auto new_binding = std::make_shared<detail::VariableBinding>(std::string(name));
        auto [inserted_it, inserted] = s_variables.try_emplace(std::string(name), std::move(new_binding));
        inserted_it->second->validate_with<&Variable::validate>(variable);
        inserted_it->second->set_default(std::string(variable.to_string()));
        variable.attach(inserted_it->second);
        return *inserted_it->second;
    }
    else {
        variable.attach(it->second);
        return *it->second;
    }
}

cvar::detail::CommandBinding& cvar::bind_cmd(std::string_view name)
{
    auto it = s_commands.find(name);

    if(it == s_commands.cend()) {
        auto new_binding = std::make_shared<detail::CommandBinding>(std::string(name));
        auto [inserted_it, inserted] = s_commands.try_emplace(std::string(name), std::move(new_binding));
        return *inserted_it->second;
    }
    else {
        return *it->second;
    }
}

cvar::detail::VariableBinding* cvar::find_var(std::string_view name)
{
    auto it = s_variables.find(name);

    if(it == s_variables.end()) {
        return nullptr;
    }
    else {
        return it->second.get();
    }
}

cvar::detail::CommandBinding* cvar::find_cmd(std::string_view name)
{
    auto it = s_commands.find(name);

    if(it == s_commands.end()) {
        return nullptr;
    }
    else {
        return it->second.get();
    }
}

cvar::status cvar::exec(std::string_view line, const Context& ctx)
{
    auto tokens = utils::tokenize_esc(line);

    if(tokens.empty()) {
        return status::ARGUMENT;
    }

    std::vector<std::string_view> views;
    views.reserve(tokens.size());

    for(const auto& token : tokens) {
        views.emplace_back(token);
    }

    auto name = views.front();
    auto args = std::span(views).subspan(1);
    return exec(name, args, ctx);
}

cvar::status cvar::exec(std::string_view name, std::span<const std::string_view> args, const Context& ctx)
{
    auto is_remote = static_cast<bool>(ctx.client_id);

    if(auto command = find_cmd(name)) {
        auto flags = command->flags().value_or(FCVAR_NONE);
        auto is_admin = static_cast<bool>(flags & FCVAR_ADMIN);
        auto is_recall = static_cast<bool>(flags & FCVAR_RECALL);

        if(!is_remote && command->is_remote() && s_remote_exec) {
            return s_remote_exec(rebuild_line(name, args));
        }

        if(is_admin && !is_trusted(ctx)) {
            msg::log_warning("{}: operator access required", name);

            return status::UNKNOWN;
        }

        if(is_remote && !is_recall) {
            msg::log_warning("{} is not allowed to be executed remotely", name);

            return status::UNKNOWN;
        }

        return command->invoke(ctx, args);
    }

    if(auto variable = find_var(name)) {
        auto flags = variable->flags().value_or(FCVAR_NONE);
        auto is_admin = static_cast<bool>(flags & FCVAR_ADMIN);
        auto is_readonly = static_cast<bool>(flags & FCVAR_READONLY);

        if(args.empty()) {
            msg::log_info("{} = {}", name, variable->value());

            return status::OK;
        }

        if(is_admin && !is_trusted(ctx)) {
            msg::log_warning("{}: operator access required", name);

            return status::UNKNOWN;
        }

        if(is_remote) {
            msg::log_warning("{} is not allowed to be set remotely", name);

            return status::UNKNOWN;
        }

        if(variable->is_shadowed()) {
            msg::log_warning("{} is replicated from the server and cannot be changed while connected", name);

            return status::UNKNOWN;
        }

        if(is_readonly) {
            msg::log_warning("{} is read-only", name);

            return status::UNKNOWN;
        }

        auto value = utils::join(args, " ");

        if(!variable->set_value(value)) {
            msg::log_warning("{}: value {} was rejected by connected validator", name, value);

            return status::ARGUMENT;
        }

        return status::OK;
    }

    msg::log_warning("{}: unknown command or variable", name);

    return status::UNKNOWN;
}

bool cvar::exec_from(std::string_view path)
{
    std::string source;

    if(!utils::read_file(path, source)) {
        return false;
    }

    std::istringstream stream(source);
    std::string line;

    while(std::getline(stream, line)) {
        auto trimmed = utils::trim_whitespace(utils::remove_comments(line));

        if(trimmed.empty()) {
            continue;
        }

        exec(trimmed);
    }

    return true;
}

void cvar::save_values(std::string_view path)
{
    std::ostringstream stream;

    for(auto& [name, binding] : s_variables) {
        auto flags = binding->flags().value_or(FCVAR_NONE);

        if(binding->is_remote()) {
            continue;
        }

        if(flags & FCVAR_ARCHIVE) {
            auto value = binding->local_value();

            if(value.size()) {
                if(utils::has_whitespace(value)) {
                    stream << std::format("{} \"{}\"", name, utils::escape_quotations(value));
                    stream << std::endl;
                }
                else {
                    stream << std::format("{} {}", name, value);
                    stream << std::endl;
                }
            }
        }
    }

    for(auto& hook : s_write_hooks) {
        hook(stream);
    }

    utils::write_file(path, stream.str());
}

void cvar::for_each_var(flag_bits mask, std::function<void(const detail::VariableBinding&)> callback)
{
    assert(callback);

    for(auto& [name, binding] : s_variables) {
        auto flags = binding->flags().value_or(FCVAR_NONE);

        if(mask == FCVAR_NONE || (flags & mask)) {
            callback(*binding);
        }
    }
}

void cvar::for_each_cmd(flag_bits mask, std::function<void(const detail::CommandBinding&)> callback)
{
    assert(callback);

    for(auto& [name, binding] : s_commands) {
        auto flags = binding->flags().value_or(FCVAR_NONE);

        if(mask == FCVAR_NONE || (flags & mask)) {
            callback(*binding);
        }
    }
}
