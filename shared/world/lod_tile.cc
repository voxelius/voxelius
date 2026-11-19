#include "shared/pch.hh"

#include "shared/world/lod_tile.hh"

#include "core/buffer.hh"

bool LodKey::operator==(const LodKey& other) const
{
    auto result = true;
    result = result && realm == other.realm;
    result = result && level == other.level;
    result = result && x == other.x;
    result = result && z == other.z;
    return result;
}

bool LodTile::encode(const LodTile& tile, WriteBuffer& buffer)
{
    WriteBuffer raw;

    raw.write<std::uint8_t>(LOD_TILE_VERSION);
    raw.write<std::uint8_t>(tile.color_mode == lod::COLOR_RGB332 ? lod::COLOR_RGB332 : lod::COLOR_PALETTE8);
    raw.write<std::uint16_t>(static_cast<std::uint16_t>(tile.palette.size()));

    for(const auto& color : tile.palette) {
        raw.write<std::uint8_t>(color[0]);
        raw.write<std::uint8_t>(color[1]);
        raw.write<std::uint8_t>(color[2]);
    }

    for(const auto& cell : tile.cells) {
        raw.write<std::int16_t>(cell.height);
        raw.write<std::int16_t>(cell.bottom);
        raw.write<std::uint8_t>(cell.color);
        raw.write<std::uint8_t>(cell.flags);
    }

    buffer.write_compressed(std::span(raw.data(), raw.size()));
    return true;
}

bool LodTile::decode(LodTile& tile, ReadBuffer& buffer)
{
    auto bytes = buffer.read_compressed();

    if(bytes.empty()) {
        return false;
    }

    ReadBuffer raw;
    raw.reset(bytes);

    constexpr auto header_size = sizeof(std::uint8_t) * 2 + sizeof(std::uint16_t);

    if(!raw.has_records(1, header_size)) {
        return false;
    }

    auto version = raw.read<std::uint8_t>();
    auto color_mode = raw.read<std::uint8_t>();
    auto palette_count = raw.read<std::uint16_t>();

    if(version != LOD_TILE_VERSION) {
        return false;
    }

    if(color_mode != lod::COLOR_PALETTE8 && color_mode != lod::COLOR_RGB332) {
        return false;
    }

    if(palette_count > lod::PALETTE_MAX) {
        return false;
    }

    auto payload_size = static_cast<std::size_t>(palette_count) * 3 + LOD_TILE_CELL_COUNT * 6;

    if(!raw.has_records(1, payload_size)) {
        return false;
    }

    tile.color_mode = color_mode;
    tile.palette.clear();
    tile.palette.reserve(palette_count);

    for(std::uint16_t i = 0; i < palette_count; ++i) {
        std::array<std::uint8_t, 3> color;
        color[0] = raw.read<std::uint8_t>();
        color[1] = raw.read<std::uint8_t>();
        color[2] = raw.read<std::uint8_t>();
        tile.palette.push_back(color);
    }

    for(auto& cell : tile.cells) {
        cell.height = raw.read<std::int16_t>();
        cell.bottom = raw.read<std::int16_t>();
        cell.color = raw.read<std::uint8_t>();
        cell.flags = raw.read<std::uint8_t>();
    }

    return true;
}
