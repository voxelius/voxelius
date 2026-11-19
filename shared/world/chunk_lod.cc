#include "shared/pch.hh"

#include "shared/world/chunk_lod.hh"

#include "core/buffer.hh"

constexpr static std::array<std::size_t, 5> LEVEL_VOLUMES = { 4096, 512, 64, 8, 1 };

std::size_t chunk_lod_level_volume(std::uint8_t level)
{
    if(level < CHUNK_LOD_LEVEL_MIN || level > CHUNK_LOD_LEVEL_MAX) {
        return 0;
    }

    return LEVEL_VOLUMES[level - CHUNK_LOD_LEVEL_MIN];
}

bool ChunkLod::encode(const ChunkLod& chunk_lod, WriteBuffer& buffer)
{
    WriteBuffer raw;

    raw.write<std::uint8_t>(CHUNK_LOD_VERSION);
    raw.write<std::uint8_t>(chunk_lod.color_mode == lod::COLOR_RGB332 ? lod::COLOR_RGB332 : lod::COLOR_PALETTE8);
    raw.write<std::uint16_t>(static_cast<std::uint16_t>(chunk_lod.palette.size()));

    for(const auto& color : chunk_lod.palette) {
        raw.write<std::uint8_t>(color[0]);
        raw.write<std::uint8_t>(color[1]);
        raw.write<std::uint8_t>(color[2]);
    }

    for(std::uint8_t level = CHUNK_LOD_LEVEL_MIN; level <= CHUNK_LOD_LEVEL_MAX; level += 1) {
        for(const auto& cell : chunk_lod.cells(level)) {
            raw.write<std::uint8_t>(cell.color);
            raw.write<std::uint8_t>(cell.coverage_flags);
        }
    }

    buffer.write_compressed(std::span(raw.data(), raw.size()));
    return true;
}

bool ChunkLod::decode(ChunkLod& chunk_lod, ReadBuffer& buffer)
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

    if(version != CHUNK_LOD_VERSION) {
        return false;
    }

    if(color_mode != lod::COLOR_PALETTE8 && color_mode != lod::COLOR_RGB332) {
        return false;
    }

    if(palette_count > lod::PALETTE_MAX) {
        return false;
    }

    auto payload_size = static_cast<std::size_t>(palette_count) * 3 + CHUNK_LOD_TOTAL_CELLS * 2;

    if(!raw.has_records(1, payload_size)) {
        return false;
    }

    chunk_lod.color_mode = color_mode;
    chunk_lod.palette.clear();
    chunk_lod.palette.reserve(palette_count);

    for(std::uint16_t i = 0; i < palette_count; ++i) {
        std::array<std::uint8_t, 3> color;
        color[0] = raw.read<std::uint8_t>();
        color[1] = raw.read<std::uint8_t>();
        color[2] = raw.read<std::uint8_t>();
        chunk_lod.palette.push_back(color);
    }

    for(std::uint8_t level = CHUNK_LOD_LEVEL_MIN; level <= CHUNK_LOD_LEVEL_MAX; level += 1) {
        for(auto& cell : chunk_lod.cells(level)) {
            cell.color = raw.read<std::uint8_t>();
            cell.coverage_flags = raw.read<std::uint8_t>();
        }
    }

    return true;
}

std::span<ChunkLodCell> ChunkLod::cells(std::uint8_t level)
{
    switch(level) {
        case 0:
            return std::span(level0);

        case 1:
            return std::span(level1);

        case 2:
            return std::span(level2);

        case 3:
            return std::span(level3);

        case 4:
            return std::span(level4);

        default:
            break;
    }

    return {};
}

std::span<const ChunkLodCell> ChunkLod::cells(std::uint8_t level) const
{
    switch(level) {
        case 0:
            return std::span(level0);

        case 1:
            return std::span(level1);

        case 2:
            return std::span(level2);

        case 3:
            return std::span(level3);

        case 4:
            return std::span(level4);

        default:
            break;
    }

    return {};
}
