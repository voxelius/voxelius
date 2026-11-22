#include "shared/pch.hh"

#include "shared/cvar/commands.hh"

#include "core/cvar/bits.hh"
#include "core/cvar/host.hh"
#include "core/msg/log.hh"
#include "core/utils/string.hh"

static cvar::status cmd_list_vars(const cvar::Context&, std::span<const std::string_view> args)
{
    thread_local std::string buffer;

    cvar::for_each_var(FCVAR_NONE, [&](const cvar::detail::VariableBinding& binding) {
        auto name = binding.name();

        if(args.size() && !name.contains(args[0])) {
            return;
        }

        auto& flags = binding.flags();
        auto& help = binding.help();

        cvar::make_bitstring_short(flags.value_or(FCVAR_NONE), buffer);

        if(help.has_value()) {
            msg::log_info("[] {:<24} [{}] {}", binding.name(), buffer, help.value());
        }
        else {
            msg::log_info("[] {:<24} [{}]", binding.name(), buffer);
        }
    });

    return cvar::status::OK;
}

static cvar::status cmd_list_cmds(const cvar::Context&, std::span<const std::string_view> args)
{
    thread_local std::string buffer;

    cvar::for_each_cmd(FCVAR_NONE, [&](const cvar::detail::CommandBinding& binding) {
        auto name = binding.name();

        if(args.size() && !name.contains(args[0])) {
            return;
        }

        auto& flags = binding.flags();
        auto& help = binding.help();

        cvar::make_bitstring_short(flags.value_or(FCVAR_NONE), buffer);

        if(help.has_value()) {
            msg::log_info("[] {:<24} [{}] {}", binding.name(), buffer, help.value());
        }
        else {
            msg::log_info("[] {:<24} [{}]", binding.name(), buffer);
        }
    });

    return cvar::status::OK;
}

static cvar::status cmd_help(const cvar::Context&, std::span<const std::string_view> args)
{
    thread_local std::string buffer;

    if(args.empty()) {
        msg::log_warning("usage: help <name>");

        return cvar::status::ARGUMENT;
    }

    cvar::flag_bits flags;
    std::string_view help;

    if(auto variable = cvar::find_var(args[0])) {
        auto& variable_flags = variable->flags();

        flags = variable_flags.value_or(FCVAR_NONE);

        if(auto& help_variant = variable->help()) {
            help = help_variant.value();
        }
    }
    else if(auto command = cvar::find_cmd(args[0])) {
        auto& command_flags = command->flags();

        flags = command_flags.value_or(FCVAR_NONE);

        if(auto& help_variant = command->help()) {
            help = help_variant.value();
        }
    }
    else {
        msg::log_warning("{}: missing argument", args[0]);

        return cvar::status::ARGUMENT;
    }

    cvar::make_bitstring_long(flags, buffer);

    if(help.size()) {
        msg::log_info("{}: {}\n  {}", args[0], buffer, help);
    }
    else {
        msg::log_info("{}: {}", args[0], buffer);
    }

    return cvar::status::OK;
}

static cvar::status cmd_echo(const cvar::Context&, std::span<const std::string_view> args)
{
    msg::log_info("{}", utils::join(args, " "));

    return cvar::status::OK;
}

void cvar::register_commands_core(void)
{
    auto& list_binding = cvar::bind_cmd("list_vars");
    list_binding.set_help("List all variables");
    list_binding.connect<&cmd_list_vars>();

    auto& list_cmds_binding = cvar::bind_cmd("list_cmds");
    list_cmds_binding.set_help("List all commands");
    list_cmds_binding.connect<&cmd_list_cmds>();

    auto& help_binding = cvar::bind_cmd("help");
    help_binding.set_help("Show help for a variable or command");
    help_binding.connect<&cmd_help>();

    auto& echo_binding = cvar::bind_cmd("echo");
    echo_binding.set_help("Echo arguments to console");
    echo_binding.connect<&cmd_echo>();
}
