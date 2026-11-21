#include "shared/pch.hh"

#include "shared/world/world_visuals.hh"

#include "core/buffer.hh"

void WorldVisuals::encode(const WorldVisuals& visuals, WriteBuffer& buffer)
{
    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(visuals.sky.has_value()));

    if(visuals.sky) {
        buffer.write_vector<float, 3>(visuals.sky->day.horizon);
        buffer.write_vector<float, 3>(visuals.sky->day.zenith);
        buffer.write_vector<float, 3>(visuals.sky->night.horizon);
        buffer.write_vector<float, 3>(visuals.sky->night.zenith);
        buffer.write<float>(visuals.sky->twilight_height);
    }

    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(visuals.sun.has_value()));

    if(visuals.sun) {
        buffer.write<float>(visuals.sun->tilt);
    }

    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(visuals.moon.has_value()));

    if(visuals.moon) {
        buffer.write_vector<float, 3>(visuals.moon->color);
    }

    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(visuals.stars.has_value()));

    if(visuals.stars) {
        buffer.write<std::uint32_t>(visuals.stars->count);
    }

    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(visuals.clouds.has_value()));
}

void WorldVisuals::decode(WorldVisuals& visuals, ReadBuffer& buffer)
{
    visuals = {};

    if(buffer.read<std::uint8_t>()) {
        Sky sky {};

        sky.day.horizon = buffer.read_vector<float, 3>();
        sky.day.zenith = buffer.read_vector<float, 3>();
        sky.night.horizon = buffer.read_vector<float, 3>();
        sky.night.zenith = buffer.read_vector<float, 3>();
        sky.twilight_height = buffer.read<float>();

        visuals.sky = sky;
    }

    if(buffer.read<std::uint8_t>()) {
        Sun sun {};

        sun.tilt = buffer.read<float>();

        visuals.sun = sun;
    }

    if(buffer.read<std::uint8_t>()) {
        Moon moon {};

        moon.color = buffer.read_vector<float, 3>();

        visuals.moon = moon;
    }

    if(buffer.read<std::uint8_t>()) {
        Stars stars {};

        stars.count = buffer.read<std::uint32_t>();

        visuals.stars = stars;
    }

    if(buffer.read<std::uint8_t>()) {
        visuals.clouds = Clouds {};
    }
}
