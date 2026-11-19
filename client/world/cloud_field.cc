#include "client/pch.hh"

#include "client/world/cloud_field.hh"

#include "shared/constant.hh"
#include "shared/utils/biome.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"

constexpr static int CLOUD_COLUMN_MAX_VOXELS = 16;
constexpr static int CLOUD_COLUMN_MAX_STEP = 2;
constexpr static float CLOUD_SHADE_VARIATION = 0.16f; // Enough to break a deck into blocks, small enough that no cube stands out

// The deck reaches no further than this, whatever r_cloud_range says; the
// rings tile the range below it.
constexpr static float CLOUD_MAX_RADIUS_BLOCKS = static_cast<float>(constant::CHUNK_SIZE * cloud_field::MAX_RANGE_CHUNKS);

constexpr static CloudRing CLOUD_RINGS[] = {
    { 8.0f, 4.0f, static_cast<float>(constant::CHUNK_SIZE * 32) },
    { 32.0f, 8.0f, static_cast<float>(constant::CHUNK_SIZE * 96) },
    { 128.0f, 16.0f, CLOUD_MAX_RADIUS_BLOCKS },
};

constexpr static float CLOUD_FINE_CELL_BLOCKS = CLOUD_RINGS[0].cell;

std::span<const CloudRing> cloud_field::rings(void)
{
    return CLOUD_RINGS;
}

float cloud_field::max_radius_blocks(void)
{
    return CLOUD_MAX_RADIUS_BLOCKS;
}

float cloud_field::fine_cell_blocks(void)
{
    return CLOUD_FINE_CELL_BLOCKS;
}

CloudDeck cloud_field::make_deck(const CloudInput& input, const CloudGroup& group, const CloudRing& ring)
{
    CloudDeck deck {};
    deck.realm = group.realm;
    deck.type = group.type;
    deck.body = group.body;
    deck.cell = ring.cell;
    deck.height = ring.height;

    auto realm_index = static_cast<std::size_t>(group.realm);
    auto offset = input.offsets[realm_index];

    deck.state = &input.states[realm_index];
    deck.anchor = input.anchors[realm_index];
    deck.center = input.center - Eigen::Vector3f(offset.x(), 0.0f, offset.y());

    deck.scale = std::max(group.body.scale, 8.0f);
    deck.max_levels = std::clamp(static_cast<int>(group.body.thickness / deck.height), 1, CLOUD_COLUMN_MAX_VOXELS);
    deck.opacity = std::clamp(group.body.opacity, 0.05f, 1.0f);
    deck.shadow = group.body.shadow_color;
    deck.lit = group.body.lit_color;
    deck.side = (deck.shadow + deck.lit) * 0.5f;

    return deck;
}

// A coarse cell stands for the whole patch the finer ring resolves: averaging
// its corners keeps the two rings on the same structure, where a point sample
// would give the coarse ring a pattern the fine one never draws.
static float cell_coverage(const CloudDeck& deck, float field_x, float field_z)
{
    auto step = deck.cell * 0.25f;
    auto state = *deck.state;

    auto a = weather::coverage_in_field(state, deck.realm, deck.type, field_x - step, field_z - step);
    auto b = weather::coverage_in_field(state, deck.realm, deck.type, field_x + step, field_z - step);
    auto c = weather::coverage_in_field(state, deck.realm, deck.type, field_x - step, field_z + step);
    auto d = weather::coverage_in_field(state, deck.realm, deck.type, field_x + step, field_z + step);

    return (a + b + c + d) * 0.25f;
}

static float column_coverage(const CloudDeck& deck, float field_x, float field_z)
{
    if(deck.cell > CLOUD_RINGS[0].cell) {
        return cell_coverage(deck, field_x, field_z);
    }

    return weather::coverage_in_field(*deck.state, deck.realm, deck.type, field_x, field_z);
}

static float shell_hash(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t seed)
{
    auto h = static_cast<std::uint32_t>(x) * UINT32_C(374761393);
    h += static_cast<std::uint32_t>(y) * UINT32_C(3266489917);
    h += static_cast<std::uint32_t>(z) * UINT32_C(668265263);
    h += seed * UINT32_C(2246822519);

    h ^= h >> 13;
    h *= UINT32_C(1274126177);
    h ^= h >> 16;

    return static_cast<float>(h & UINT32_C(0xFFFFFF)) / static_cast<float>(UINT32_C(0xFFFFFF));
}

// A 3D value noise, used only to break the top of a column into heaps.
// The weather field itself stays 2D, so the front and the seed remain shared.
static float shell_noise(float x, float y, float z, std::uint32_t seed)
{
    auto x0 = static_cast<std::int32_t>(std::floor(x));
    auto y0 = static_cast<std::int32_t>(std::floor(y));
    auto z0 = static_cast<std::int32_t>(std::floor(z));

    auto fx = x - static_cast<float>(x0);
    auto fy = y - static_cast<float>(y0);
    auto fz = z - static_cast<float>(z0);

    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);

    auto a = shell_hash(x0, y0, z0, seed) * (1.0f - fy) + shell_hash(x0, y0 + 1, z0, seed) * fy;
    auto b = shell_hash(x0 + 1, y0, z0, seed) * (1.0f - fy) + shell_hash(x0 + 1, y0 + 1, z0, seed) * fy;
    auto c = shell_hash(x0, y0, z0 + 1, seed) * (1.0f - fy) + shell_hash(x0, y0 + 1, z0 + 1, seed) * fy;
    auto d = shell_hash(x0 + 1, y0, z0 + 1, seed) * (1.0f - fy) + shell_hash(x0 + 1, y0 + 1, z0 + 1, seed) * fy;

    auto top = a * (1.0f - fx) + b * fx;
    auto bottom = c * (1.0f - fx) + d * fx;

    return top * (1.0f - fz) + bottom * fz;
}

CloudColumn cloud_field::sample_column(const CloudDeck& deck, std::int32_t cell_x, std::int32_t cell_z)
{
    auto grid_x = deck.cell * (static_cast<float>(cell_x) + 0.5f);
    auto grid_z = deck.cell * (static_cast<float>(cell_z) + 0.5f);

    // The field is sampled in its own frame, so a cell keeps its
    // answer at every rebuild however far the front has drifted.
    // Going through world space and subtracting the drift again
    // would round differently each time and flip cubes near a threshold
    auto field_x = grid_x - deck.anchor.x();
    auto field_z = grid_z - deck.anchor.y();

    auto coverage = column_coverage(deck, field_x, field_z) * deck.body.coverage;

    if(coverage <= 0.02f) {
        return CloudColumn {};
    }

    // Two coarse octaves: the broad one carries the weight of the deck, the
    // smaller one keeps a thin deck from collapsing.
    auto clump = weather::noise_at(field_x / (deck.scale * 2.0f), field_z / (deck.scale * 2.0f), deck.body.seed + 5);
    auto mass = weather::noise_at(field_x / deck.scale, field_z / deck.scale, deck.body.seed);
    auto clump_weight = 0.45f + 0.30f * std::clamp(coverage, 0.0f, 1.0f);
    auto density = clump * clump_weight + mass * (1.0f - clump_weight);

    // A thin type keeps only its densest lumps, an overcast keeps almost the whole field
    auto threshold = 0.62f - std::clamp(coverage, 0.0f, 1.0f) * 0.22f;
    auto headroom = density - threshold;

    // The per-cell crumble breaks a thin deck into scraps and frays the rim
    // of a mass, and leaves the inside of a dense deck clean.
    auto hash_x = static_cast<std::int32_t>(std::floor(field_x / deck.cell));
    auto hash_z = static_cast<std::int32_t>(std::floor(field_z / deck.cell));
    auto open = 1.0f - std::clamp(coverage, 0.0f, 1.0f);
    auto seam = 1.0f - std::clamp(headroom / 0.18f, 0.0f, 1.0f);
    auto bias = weather::cell_random(hash_x, hash_z, deck.body.seed + 17) - 0.5f;
    auto value = headroom + bias * 0.5f * std::max(open, seam * 0.4f);

    if(value <= 0.0f) {
        return CloudColumn {};
    }

    // A thin front stays a sheet; a dense one grows into the body's full
    // thickness, and the shell noise breaks the top into heaps.
    auto rise = std::clamp(value / 0.30f, 0.0f, 1.0f);
    auto shell_y = deck.body.height + deck.body.thickness * 0.5f;
    auto shell = shell_noise(field_x / (deck.scale * 0.55f), shell_y / (deck.scale * 0.55f), field_z / (deck.scale * 0.55f),
        deck.body.seed + 73);
    auto dome = deck.body.thickness * (0.30f + 0.70f * rise) * (0.72f + 0.56f * shell);
    auto levels = std::clamp(static_cast<int>(std::lround(dome / deck.height)), 1, deck.max_levels);

    // The underside steps up into the volume here and there, so the deck reads
    // as a volume from below, but never past the body's own slab: a deck that
    // grew past it would fight the body above it for the same pixels.
    auto step_max = std::min(CLOUD_COLUMN_MAX_STEP, levels - 1);
    auto step = static_cast<int>(std::lround(clump * static_cast<float>(step_max)));

    // The column's own brightness, kept in the field's frame like everything
    // else so a rebuild never repaints the deck.
    auto shade = 1.0f + (weather::cell_random(hash_x, hash_z, deck.body.seed + 41) - 0.5f) * CLOUD_SHADE_VARIATION;

    CloudColumn column {};
    column.bottom = static_cast<std::int16_t>(step);
    column.top = static_cast<std::int16_t>(levels);
    column.shade = shade;

    return column;
}

static void add_group(biome_realm realm, weather_type_id type, const CloudBody& body, float camera_y, std::vector<CloudGroup>& out)
{
    if(body.coverage <= 0.02f) {
        return;
    }

    // A deck belongs to the realm its height sits in; otherwise a type shared
    // by two realms would be built twice in place.
    if(utils::realm_of_y(body.height) != realm) {
        return;
    }

    CloudGroup group {};
    group.realm = realm;
    group.type = type;
    group.body = body;
    group.distance = std::abs(body.height - camera_y);

    out.push_back(group);
}

static void collect_realm_groups(biome_realm realm, float camera_y, std::vector<CloudGroup>& out)
{
    auto bit = 1U << static_cast<unsigned>(realm);

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL || 0 == (def.realms_bits & bit)) {
            continue;
        }

        add_group(realm, def.id, def.cloud, camera_y, out);
    }
}

void cloud_field::collect_groups(float camera_y, std::vector<CloudGroup>& out)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        collect_realm_groups(static_cast<biome_realm>(i), camera_y, out);
    }
}
