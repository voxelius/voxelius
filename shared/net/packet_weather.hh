#ifndef D9F5A4C0_7E63_4F1D_8B49_5C1D3E8F2A74
#define D9F5A4C0_7E63_4F1D_8B49_5C1D3E8F2A74

#include "shared/net/packet.hh"
#include "shared/world/biome.hh"
#include "shared/world/weather.hh"

struct packet::Weather_RealmState final : public packet::Base<packet::WORLD_WEATHER> {
    struct Realm final {
        Eigen::Vector2f types_drift { Eigen::Vector2f::Zero() };
        float types_scale { 1024.0f };
        std::uint32_t types_seed { 0 };

        Eigen::Vector2f detail_drift { Eigen::Vector2f::Zero() };
        float detail_scale { 512.0f };
        std::uint32_t detail_seed { 0 };

        Eigen::Vector2f wind { Eigen::Vector2f::Zero() };
    };

    static void encode(const Weather_RealmState& packet, WriteBuffer& buffer);
    static void decode(Weather_RealmState& packet, ReadBuffer& buffer);

    std::array<Realm, NUM_BIOME_REALMS> realms;
};

#endif /* D9F5A4C0_7E63_4F1D_8B49_5C1D3E8F2A74 */
