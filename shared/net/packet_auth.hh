#ifndef DDFD985C_53DD_4D07_BCE3_B9644A7AE004
#define DDFD985C_53DD_4D07_BCE3_B9644A7AE004

#include "shared/net/ed25519.hh"
#include "shared/net/packet.hh"

struct packet::Auth_Request final : public packet::Base<packet::AUTH_REQUEST> {
    static void encode(const Auth_Request& packet, WriteBuffer& buffer);
    static void decode(Auth_Request& packet, ReadBuffer& buffer);

    std::uint32_t version_major;
    std::uint32_t version_minor;
    std::uint32_t version_patch;
    ed25519::pkey_type pkey;
    std::uint64_t invite_code;
    std::uint64_t biomes_hash;
    std::uint64_t blocks_hash;
    std::uint64_t fluids_hash;
    std::uint64_t tints_hash;
    std::uint64_t ents_hash;
    std::uint64_t weather_hash;
    std::string username;
};

struct packet::Auth_Challenge final : public packet::Base<packet::AUTH_CHALLENGE> {
    static void encode(const Auth_Challenge& packet, WriteBuffer& buffer);
    static void decode(Auth_Challenge& packet, ReadBuffer& buffer);

    std::array<std::byte, 64> nonce;
};

struct packet::Auth_Response final : public packet::Base<packet::AUTH_RESPONSE> {
    static void encode(const Auth_Response& packet, WriteBuffer& buffer);
    static void decode(Auth_Response& packet, ReadBuffer& buffer);

    ed25519::sign_type signature;
};

struct packet::Auth_Admission final : public packet::Base<packet::AUTH_ADMISSION> {
    static void encode(const Auth_Admission& packet, WriteBuffer& buffer);
    static void decode(Auth_Admission& packet, ReadBuffer& buffer);

    std::uint16_t client_id;
    std::uint64_t identity;
    std::string username;
};

#endif /* DDFD985C_53DD_4D07_BCE3_B9644A7AE004 */
