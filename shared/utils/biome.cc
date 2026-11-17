#include "shared/pch.hh"

#include "shared/utils/biome.hh"

#include "shared/constant.hh"

static bool check_range(ChunkPos::value_type y, ChunkPos::value_type min, ChunkPos::value_type max)
{
    return y >= min && y <= max;
}

biome_realm utils::realm(ChunkPos::value_type y)
{
    if(check_range(y, constant::SURFACE_MIN_CHUNK_Y, constant::SURFACE_MAX_CHUNK_Y)) {
        return BIOME_REALM_SURFACE;
    }
    else if(check_range(y, constant::SKY_MIN_CHUNK_Y, constant::SKY_MAX_CHUNK_Y)) {
        return BIOME_REALM_SKY;
    }
    else if(check_range(y, constant::UNDERGROUND_MIN_CHUNK_Y, constant::UNDERGROUND_MAX_CHUNK_Y)) {
        return BIOME_REALM_UNDERGROUND;
    }
    else if(check_range(y, constant::THE_DEPTHS_MIN_CHUNK_Y, constant::THE_DEPTHS_MAX_CHUNK_Y)) {
        return BIOME_REALM_THE_DEPTHS;
    }

    return BIOME_REALM_VOID;
}

biome_realm utils::realm_of_y(float by)
{
    auto chunk_y = static_cast<ChunkPos::value_type>(std::floor(by / static_cast<float>(constant::CHUNK_SIZE)));

    return utils::realm(chunk_y);
}
