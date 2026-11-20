#ifndef D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64
#define D6B15C7A_9E42_4F10_B3A7_1C0D8E5F2A64

#include "core/identifier.hh"

#include "shared/coord.hh"
#include "shared/world/biome.hh"
#include "shared/world/weather.hh"

struct WorldVisuals final {
    Eigen::Vector3f day_horizon { 0.64f, 0.76f, 0.94f };
    Eigen::Vector3f day_zenith { 0.26f, 0.46f, 0.85f };
    Eigen::Vector3f night_horizon { 0.020f, 0.030f, 0.060f };
    Eigen::Vector3f night_zenith { 0.004f, 0.007f, 0.018f };
    Eigen::Vector3f moon_color { 0.86f, 0.90f, 1.0f };

    float sun_tilt { 0.0f };
    float twilight_height { 0.35f };

    std::uint32_t star_count { 1500 };
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
