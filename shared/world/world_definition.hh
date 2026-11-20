#ifndef D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64
#define D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64

#include "core/identifier.hh"

#include "shared/coord.hh"
#include "shared/world/biome.hh"
#include "shared/world/weather.hh"

struct WorldVisuals final {
    struct Sky final {
        struct Gradient final {
            Eigen::Vector3f horizon;
            Eigen::Vector3f zenith;
        };

        Gradient day;
        Gradient night;

        float twilight_height;
    };

    struct Sun final {
        float tilt;
    };

    struct Moon final {
        Eigen::Vector3f color;
    };

    struct Stars final {
        std::uint32_t count;
    };

    struct Clouds final {
        // empty
    };

    std::optional<Sky> sky;
    std::optional<Sun> sun;
    std::optional<Moon> moon;
    std::optional<Stars> stars;
    std::optional<Clouds> clouds;
};

struct WorldDefinition final {
    Identifier id;

    std::string directory; // Directory under save/; the id is not a path
    std::string generator; // Generator name from C++, such as "vox17"

    BlockPos::value_type min_y { -64 };
    BlockPos::value_type max_y { 255 };

    BlockPos spawn { BlockPos::Zero() };

    WorldVisuals visuals;

    std::vector<Identifier> biome_names;
    std::vector<biome_id_type> biomes;
    std::vector<Identifier> weather_names;
    std::vector<weather_type_id> weather;
};

#endif /* D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64 */
