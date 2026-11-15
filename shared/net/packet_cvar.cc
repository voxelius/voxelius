#include "shared/pch.hh"

#include "shared/net/packet_cvar.hh"

#include "core/buffer.hh"
#include "core/cvar/binding.hh"
#include "core/cvar/host.hh"

void packet::Cvar_ServerVars::encode(const Cvar_ServerVars& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint8_t>(packet.full);
    buffer.write<std::uint32_t>(static_cast<std::uint32_t>(packet.variables.size()));

    for(auto& variable : packet.variables) {
        buffer.write<std::string_view>(variable.name);
        buffer.write<std::string_view>(variable.value);

        if(packet.full) {
            buffer.write<std::uint16_t>(variable.flags);
            buffer.write<std::string_view>(variable.help);
        }
    }
}

void packet::Cvar_ServerVars::decode(Cvar_ServerVars& packet, ReadBuffer& buffer)
{
    packet.full = buffer.read<std::uint8_t>();
    packet.variables.resize(buffer.read<std::uint32_t>());

    for(auto& variable : packet.variables) {
        variable.name = buffer.read<std::string>();
        variable.value = buffer.read<std::string>();

        if(packet.full) {
            variable.flags = buffer.read<std::uint16_t>();
            variable.help = buffer.read<std::string>();
        }
        else {
            variable.flags = FCVAR_NONE;
            variable.help.clear();
        }
    }
}

void packet::Cvar_ServerVars::build_full(Cvar_ServerVars& packet)
{
    packet.full = true;
    packet.variables.clear();

    cvar::for_each_var(FCVAR_REPLICATED, [&](const cvar::detail::VariableBinding& binding) {
        auto& variable = packet.variables.emplace_back();
        variable.name = std::string(binding.name());
        variable.value = std::string(binding.value());
        variable.flags = binding.flags().value_or(FCVAR_NONE);

        if(auto& help = binding.help()) {
            variable.help = help.value();
        }
    });
}

void packet::Cvar_ServerVars::build_dirty(std::span<const std::string> names, Cvar_ServerVars& packet)
{
    packet.full = false;
    packet.variables.clear();

    for(auto& name : names) {
        auto binding = cvar::find_var(name);

        if(binding == nullptr) {
            continue;
        }

        auto flags = binding->flags().value_or(FCVAR_NONE);

        if(flags & FCVAR_REPLICATED) {
            auto& variable = packet.variables.emplace_back();
            variable.name = name;
            variable.value = std::string(binding->value());
            variable.flags = FCVAR_NONE;
        }
    }
}

void packet::Cvar_ServerCmds::encode(const Cvar_ServerCmds& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(static_cast<std::uint32_t>(packet.commands.size()));

    for(auto& command : packet.commands) {
        buffer.write<std::string_view>(command.name);
        buffer.write<std::uint16_t>(command.flags);
        buffer.write<std::string_view>(command.help);
    }
}

void packet::Cvar_ServerCmds::decode(Cvar_ServerCmds& packet, ReadBuffer& buffer)
{
    packet.commands.resize(buffer.read<std::uint32_t>());

    for(auto& command : packet.commands) {
        command.name = buffer.read<std::string>();
        command.flags = buffer.read<std::uint16_t>();
        command.help = buffer.read<std::string>();
    }
}

void packet::Cvar_ServerCmds::build_full(Cvar_ServerCmds& packet, bool include_admin)
{
    packet.commands.clear();

    cvar::for_each_cmd(FCVAR_RECALL, [&](const cvar::detail::CommandBinding& binding) {
        auto flags = binding.flags().value_or(FCVAR_NONE);

        if(!include_admin && (flags & FCVAR_ADMIN)) {
            return;
        }

        auto& command = packet.commands.emplace_back();
        command.name = std::string(binding.name());
        command.flags = flags;

        if(auto& help = binding.help()) {
            command.help = help.value();
        }
    });
}

void packet::Cvar_Command::encode(const Cvar_Command& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(packet.request_id);
    buffer.write<std::string_view>(std::string_view(packet.line).substr(0, MAX_LINE));
}

void packet::Cvar_Command::decode(Cvar_Command& packet, ReadBuffer& buffer)
{
    packet.request_id = buffer.read<std::uint32_t>();
    packet.line = buffer.read<std::string>().substr(0, MAX_LINE);
}
