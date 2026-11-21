#ifndef D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64
#define D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64

#include "core/identifier.hh"

#include "shared/coord.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/weather.hh"
#include "shared/world/world_visuals.hh"

struct WorldDefinition final {
    Identifier id;

    std::string directory; // Directory under save/; the id is not a path
    std::string generator; // Generator name from C++, such as "vox17"

    BlockPos::value_type min_y { -64 };
    BlockPos::value_type max_y { 255 };

    BlockPos spawn { BlockPos::Zero() };

    WorldVisuals visuals;

    BiomeSelector biome_selector;

    std::vector<Identifier> weather_names;
    std::vector<weather_type_id> weather;
};

#endif /* D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64 */
