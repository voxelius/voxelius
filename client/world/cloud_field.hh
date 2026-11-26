#ifndef C4F8A2D6_5E19_4B70_9C83_A1D2F4B6C8E0
#define C4F8A2D6_5E19_4B70_9C83_A1D2F4B6C8E0

#include "shared/world/weather.hh"

struct CloudRing final {
    float cell;
    float height;
    float radius;
};

// A column in cell units, relative to the layer's base; top <= bottom means
// the spot holds no cloud. The shade is this column's own brightness, so a
// deck is a mosaic of blocks instead of one flat slab
struct CloudColumn final {
    std::int16_t bottom;
    std::int16_t top;
    float shade;
};

struct CloudDeck final {
    weather_type_id type;
    CloudBody body;
    const WeatherState* state;
    std::span<const weather_type_id> allowed;
    Eigen::Vector2f anchor;
    Eigen::Vector3f center;
    float cell;
    float height;
    float scale;
    int max_levels;
    float opacity;
    Eigen::Vector3f shadow;
    Eigen::Vector3f lit;
    Eigen::Vector3f side;
};

struct CloudGroup final {
    weather_type_id type;
    // A copy, because the registry may grow between the sort and the build
    CloudBody body;
    float distance;
};

struct CloudInput final {
    WeatherState state;
    std::vector<weather_type_id> allowed;
    Eigen::Vector2f anchor;
    Eigen::Vector2f drift_offset;
    Eigen::Vector3f center;
    float radius;
};

namespace cloud_field
{
constexpr static unsigned MAX_RANGE_CHUNKS = 256;
} // namespace cloud_field

namespace cloud_field
{
std::span<const CloudRing> rings(void);
float max_radius_blocks(void);
float fine_cell_blocks(void);
} // namespace cloud_field

namespace cloud_field
{
CloudDeck make_deck(const CloudInput& input, const CloudGroup& group, const CloudRing& ring);
CloudColumn sample_column(const CloudDeck& deck, std::int32_t cell_x, std::int32_t cell_z);
void collect_groups(float camera_y, std::vector<CloudGroup>& out);
} // namespace cloud_field

#endif /* C4F8A2D6_5E19_4B70_9C83_A1D2F4B6C8E0 */
