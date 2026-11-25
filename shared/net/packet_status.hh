#ifndef A77BB035_0E49_4D48_89BF_2DE0B21B203F
#define A77BB035_0E49_4D48_89BF_2DE0B21B203F

#include "shared/net/packet.hh"

struct packet::Status_Request final : public packet::Base<packet::STATUS_REQUEST> {
    static void encode(const Status_Request& packet, WriteBuffer& buffer);
    static void decode(Status_Request& packet, ReadBuffer& buffer);

    std::uint32_t version_major;
    std::uint32_t version_minor;
    std::uint32_t version_patch;
};

struct packet::Status_Response final : public packet::Base<packet::STATUS_RESPONSE> {
    constexpr static std::uint32_t WHITELIST_ENABLED = 0x00000001;
    constexpr static std::size_t MOTD_MAX_LENGTH = 32;
    constexpr static std::uint32_t STRICT_VERSION = 0x00000002;

    static void encode(const Status_Response& packet, WriteBuffer& buffer);
    static void decode(Status_Response& packet, ReadBuffer& buffer);

    std::uint32_t version_major;
    std::uint16_t max_players;
    std::uint16_t num_players;
    std::string motd;
    std::uint32_t version_minor;
    std::uint32_t version_patch;
    std::uint32_t server_tags;
};

#endif /* A77BB035_0E49_4D48_89BF_2DE0B21B203F */
