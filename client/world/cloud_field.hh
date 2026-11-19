#ifndef C4F8A2D6_5E19_4B70_9C83_A1D2F4B6C8E0
#define C4F8A2D6_5E19_4B70_9C83_A1D2F4B6C8E0

#include "shared/world/weather.hh"

// The deck is meshed in rings: a cell is this many blocks across and this
// many tall in a ring, and the ring reaches out to this radius. Each ring
// uses four times the cell of the one inside it, so the column count grows
// with the radius instead of its square.
struct CloudRing final {
    float cell;
    float height;
    float radius;
};

// A column in cell units, relative to the layer's base; top <= bottom means
// the spot holds no cloud. The shade is this column's own brightness, so a
// deck is a mosaic of blocks instead of one flat slab.
struct CloudColumn final {
    std::int16_t bottom;
    std::int16_t top;
    float shade;
};

// One layer of one weather type in one ring, with everything the mesher
// needs worked out once.
struct CloudDeck final {
    biome_realm realm;
    weather_type_id type;
    CloudLayer layer;
    const WeatherRealmState* state;
    Eigen::Vector2f anchor; // Field drift of this deck's own realm, taken when the deck was anchored
    Eigen::Vector3f center; // World point the window is meshed around, in the frame the vertices use
    float cell;
    float height;
    float scale;
    int max_levels;
    float opacity;
    Eigen::Vector3f shadow;
    Eigen::Vector3f lit;
    Eigen::Vector3f side;
};

// A layer that is actually built, with the distance it sits at from the
// camera; the mesher draws the farthest first.
struct CloudGroup final {
    biome_realm realm;
    weather_type_id type;
    // A copy, because the registry may grow between the sort and the build
    CloudLayer layer;
    float distance;
};

// Everything a worker needs: a copy of the field state and the frame the
// window belongs to. Each realm drifts on its own, so every deck is anchored
// and moved by the realm its layer sits in, never by the camera's realm
struct CloudInput final {
    std::array<WeatherRealmState, NUM_BIOME_REALMS> states;
    std::array<Eigen::Vector2f, NUM_BIOME_REALMS> anchors;
    std::array<Eigen::Vector2f, NUM_BIOME_REALMS> offsets;
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
