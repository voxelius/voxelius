#ifndef D1058B64_4176_4F3F_8A52_75E0BAEF0668
#define D1058B64_4176_4F3F_8A52_75E0BAEF0668

#include "core/cvar/bits.hh"

#include "shared/net/packet.hh"

struct packet::Cvar_ServerVars final : public packet::Base<packet::CVAR_SERVER_VARS> {
    struct Variable final {
        std::string name;
        std::string value;
        cvar::flag_bits flags;
        std::string help;
    };

    static void encode(const Cvar_ServerVars& packet, WriteBuffer& buffer);
    static void decode(Cvar_ServerVars& packet, ReadBuffer& buffer);

    static void build_full(Cvar_ServerVars& packet);
    static void build_dirty(std::span<const std::string> names, Cvar_ServerVars& packet);

    bool full;
    std::vector<Variable> variables;
};

struct packet::Cvar_ServerCmds final : public packet::Base<packet::CVAR_SERVER_CMDS> {
    struct Command final {
        std::string name;
        cvar::flag_bits flags;
        std::string help;
    };

    static void encode(const Cvar_ServerCmds& packet, WriteBuffer& buffer);
    static void decode(Cvar_ServerCmds& packet, ReadBuffer& buffer);

    static void build_full(Cvar_ServerCmds& packet, bool include_admin);

    std::vector<Command> commands;
};

struct packet::Cvar_Command final : public packet::Base<packet::CVAR_COMMAND> {
    constexpr static std::size_t MAX_LINE = 512;

    static void encode(const Cvar_Command& packet, WriteBuffer& buffer);
    static void decode(Cvar_Command& packet, ReadBuffer& buffer);

    std::uint32_t request_id;
    std::string line;
};

#endif /* D1058B64_4176_4F3F_8A52_75E0BAEF0668 */
