#ifndef BCF648BF_4565_4F82_9C42_0491F751D447
#define BCF648BF_4565_4F82_9C42_0491F751D447

#include "core/identifier.hh"
#include "core/res/resource.hh"

#include "shared/world/block.hh"

struct Feature;

using biome_id_type = std::uint32_t;
constexpr static biome_id_type BIOME_ID_NULL = 0;
constexpr static biome_id_type BIOME_ID_MAX = std::numeric_limits<biome_id_type>::max();

struct BiomeScatterEntry final {
    Identifier feature;
    float chance;
    block_tag_bit need_above;
    block_tag_bit need_below;
    std::size_t group_hash;
    unsigned padding;
    unsigned edge;

    res::handle<Feature> cached;
};

struct BiomePaletteEntry final {
    Identifier name;
    vx::hash_map<std::string, std::string> states;
    block_id_type cached;
};

struct BiomeDefinition {
    BiomeDefinition(void) = default;

    std::uint8_t temperature;
    std::uint8_t humidity;
    std::uint8_t continentalness;
    std::uint8_t erosion;
    std::uint8_t weirdness;

    unsigned priority;
    float offset;

    BiomePaletteEntry palette_basic;
    BiomePaletteEntry palette_filler;
    BiomePaletteEntry palette_surface;
    BiomePaletteEntry palette_fluid;

    std::vector<BiomeScatterEntry> scatter;

    vx::hash_map<Identifier, Eigen::Vector3f> tint_map;
    std::vector<Eigen::Vector3f> tint_colors;
};

#endif /* BCF648BF_4565_4F82_9C42_0491F751D447 */
