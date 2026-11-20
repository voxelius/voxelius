#ifndef BE0B58E2_0871_46F1_830D_2C8D9E4967DD
#define BE0B58E2_0871_46F1_830D_2C8D9E4967DD

#include "shared/net/packet.hh"

struct packet::Session_Disconnect final : public packet::Base<packet::SESSION_DISCONNECT> {
    constexpr static std::uint32_t UNSPECIFIED = 0x00000000;
    constexpr static std::uint32_t CLIENT_DISCONNECT = 0x00000001;
    constexpr static std::uint32_t CLIENT_SHUTDOWN = 0x00000002;
    constexpr static std::uint32_t ENTITY_ID_DESYNC = 0x00000003;
    constexpr static std::uint32_t CHECKSUM_MISMATCH = 0x00000004;
    constexpr static std::uint32_t NOT_WHITELISTED = 0x00000005;
    constexpr static std::uint32_t INVALID_SIGNATURE = 0x00000006;
    constexpr static std::uint32_t OUTDATED_CLIENT = 0x00000007;
    constexpr static std::uint32_t OUTDATED_SERVER = 0x00000008;
    constexpr static std::uint32_t SERVER_IS_FULL = 0x00000009;
    constexpr static std::uint32_t SERVER_SHUTDOWN = 0x0000000A;
    constexpr static std::uint32_t CVAR_REJECTED = 0x0000000B;
    constexpr static std::uint32_t KICKED = 0x0000000C;
    constexpr static std::uint32_t WORLD_UNKNOWN = 0x0000000D;

    static void encode(const Session_Disconnect& packet, WriteBuffer& buffer);
    static void decode(Session_Disconnect& packet, ReadBuffer& buffer);

    static std::string_view reason_string_client(std::uint32_t reason);
    static std::string_view reason_string_server(std::uint32_t reason);

    std::uint32_t reason;
};

#endif /* BE0B58E2_0871_46F1_830D_2C8D9E4967DD */
