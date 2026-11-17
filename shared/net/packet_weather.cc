#include "shared/pch.hh"

#include "shared/net/packet_weather.hh"

#include "core/buffer.hh"

void packet::Weather_RealmState::encode(const Weather_RealmState& packet, WriteBuffer& buffer)
{
    for(const auto& realm : packet.realms) {
        buffer.write_vector<float, 2>(realm.types_drift);
        buffer.write<float>(realm.types_scale);
        buffer.write<std::uint32_t>(realm.types_seed);
        buffer.write_vector<float, 2>(realm.detail_drift);
        buffer.write<float>(realm.detail_scale);
        buffer.write<std::uint32_t>(realm.detail_seed);
        buffer.write_vector<float, 2>(realm.wind);
    }
}

void packet::Weather_RealmState::decode(Weather_RealmState& packet, ReadBuffer& buffer)
{
    for(auto& realm : packet.realms) {
        realm.types_drift = buffer.read_vector<float, 2>();
        realm.types_scale = buffer.read<float>();
        realm.types_seed = buffer.read<std::uint32_t>();
        realm.detail_drift = buffer.read_vector<float, 2>();
        realm.detail_scale = buffer.read<float>();
        realm.detail_seed = buffer.read<std::uint32_t>();
        realm.wind = buffer.read_vector<float, 2>();
    }
}
