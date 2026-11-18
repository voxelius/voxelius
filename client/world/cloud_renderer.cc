#include "client/pch.hh"

#include "client/world/cloud_renderer.hh"

#include "core/camera.hh"
#include "core/cvar/host.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/threading.hh"
#include "core/utils/angles.hh"

#include "shared/constant.hh"
#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/settings.hh"
#include "client/shader_program.hh"
#include "client/world/skybox.hh"

constexpr static int CLOUD_COLUMN_MAX_VOXELS = 8;
constexpr static int CLOUD_COLUMN_MAX_STEP = 2;
constexpr static float CLOUD_SHADE_VARIATION = 0.16f; // Enough to break a deck into blocks, small enough that no cube stands out

// The deck is meshed in rings: a cell is this many blocks across
// and this many tall in a ring, and the ring reaches out to this
// radius. Each ring uses four times the cell of the one inside
// it, so the column count grows with the radius instead of its square
struct CloudRing final {
    float cell;
    float height;
    float radius;
};

// The deck reaches no further than this, whatever r_cloud_range says; the rings tile the range below it
constexpr static float CLOUD_MAX_RADIUS_BLOCKS = static_cast<float>(constant::CHUNK_SIZE * cloud_renderer::MAX_RANGE_CHUNKS);

constexpr static CloudRing CLOUD_RINGS[] = {
    { 8.0f, 4.0f, static_cast<float>(constant::CHUNK_SIZE * 32) },
    { 32.0f, 8.0f, static_cast<float>(constant::CHUNK_SIZE * 96) },
    { 128.0f, 16.0f, CLOUD_MAX_RADIUS_BLOCKS },
};

constexpr static float CLOUD_FINE_CELL_BLOCKS = CLOUD_RINGS[0].cell; // The finest ring sets the drift step a rebuild waits for
constexpr static float WIND_EASE_PER_SECOND = 6.0f;
constexpr static float BUILD_COOLDOWN_SECONDS = 0.2f; // A build closer than this to the previous one would not pay for itself
constexpr static float CLOUD_NEAR_PLANE = 1.0f;
constexpr static float CLOUD_DEPTH_MARGIN = 1024.0f;

// Colour plus the alpha the layer asks for, kept as three
// floats and a scalar: Eigen over-aligns a Vector4f to
// sixteen bytes, and that padding would break the vertex layout
struct CloudColor final {
    Eigen::Vector3f rgb;
    float alpha;
};

struct CloudVertex final {
    Eigen::Vector3f position;
    CloudColor color;
};

// A column in cell units, relative to the layer's base; top <=
// bottom means the spot holds no cloud. The shade is this column's own
// brightness, so a deck is a mosaic of blocks instead of one flat slab
struct CloudColumn final {
    std::int16_t bottom;
    std::int16_t top;
    float shade;
};

struct CloudWindow final {
    std::int32_t min_x;
    std::int32_t min_z;
    std::size_t width;
    std::size_t depth;
};

// One layer of one weather type in one ring, with everything the mesher needs worked out once
struct CloudDeck final {
    biome_realm realm;
    weather_type_id type;
    CloudLayer layer;
    const WeatherRealmState* state;
    Eigen::Vector2f anchor;
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
    biome_realm realm;
    weather_type_id type;
    // A copy, because the registry may grow between the sort and the build
    CloudLayer layer;
    float distance;
};

struct CloudBatch final {
    std::size_t offset;
    std::size_t count;
    float fade;        // Outer edge of the deck; zero for a ring inside it, which does not fade out
    float blend_start; // Outer edge of the ring, where it fades into the coarser one; a zero width means no fade
    float blend_width;
    float depth_bias; // Pushes the ring back in depth, so two rings that overlap at the same height do not fight over the pixel
};

// One build's worth of geometry; a worker
// fills it and the main thread uploads it
struct CloudMesh final {
    std::vector<CloudVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<CloudBatch> batches;

    // Center of the window the build was meshed around, in field
    // coordinates; the rings and their fades are measured from it
    Eigen::Vector2f center;
};

// Everything a worker needs: a copy of the
// field state and the frame the window belongs to
struct CloudInput final {
    std::array<WeatherRealmState, NUM_BIOME_REALMS> states;
    Eigen::Vector2f anchor;
    Eigen::Vector3f center;
    float radius;
};

static_assert(sizeof(CloudVertex) == 7 * sizeof(float), "CloudVertex must stay packed for the vertex layout");

static cvar::Number<unsigned> s_clouds { 1, 0, 1 };
static gui::CheckBox s_clouds_gui;

static ShaderProgram s_program;
static std::size_t su_CloudViewProjection;
static std::size_t su_CameraOrigin;
static std::size_t su_DeckCenter;
static std::size_t su_WindOffset;
static std::size_t su_CloudTint;
static std::size_t su_FadeDistance;
static std::size_t su_BlendRange;
static std::size_t su_DepthBias;

static GLuint s_vao[2];
static GLuint s_vbo[2];
static GLuint s_ibo[2];

static CloudMesh s_meshes[2];
static std::size_t s_front;
static std::vector<CloudGroup> s_groups;
static Eigen::Vector3f s_wind_offset { Eigen::Vector3f::Zero() };
static Eigen::Vector3f s_last_build_offset { Eigen::Vector3f::Zero() };
static std::uint64_t s_state_signature;
static std::uint64_t s_generation;
static Eigen::Vector3f s_last_build_center { Eigen::Vector3f::Zero() };
static unsigned s_last_cloud_range { 0 };
static Eigen::Vector2f s_drift_anchor { Eigen::Vector2f::Zero() };
static biome_realm s_anchor_realm { BIOME_REALM_SURFACE };
static bool s_anchored { false };
static bool s_building { false };
static float s_build_cooldown;

static Eigen::Vector3f camera_world_position(void)
{
    return utils::to_fvec(camera::chunk) + camera::local;
}

static std::size_t column_index(const CloudWindow& window, std::size_t x, std::size_t z)
{
    return z * window.width + x;
}

// A cell outside the window holds no cloud, so a neighbour past the edge simply hides nothing
static CloudColumn column_at(const CloudWindow& window, const std::vector<CloudColumn>& columns, std::int32_t x, std::int32_t z)
{
    if(x < 0 || z < 0) {
        return CloudColumn {};
    }

    if(x >= static_cast<std::int32_t>(window.width) || z >= static_cast<std::int32_t>(window.depth)) {
        return CloudColumn {};
    }

    return columns[column_index(window, static_cast<std::size_t>(x), static_cast<std::size_t>(z))];
}

static void push_vertex(CloudMesh& mesh, const Eigen::Vector3f& position, const CloudColor& color)
{
    CloudVertex vertex {};
    vertex.position = position;
    vertex.color = color;

    mesh.vertices.push_back(vertex);
}

static void push_quad(CloudMesh& mesh, const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c,
    const Eigen::Vector3f& d, const CloudColor& color)
{
    auto base = static_cast<std::uint32_t>(mesh.vertices.size());

    push_vertex(mesh, a, color);
    push_vertex(mesh, b, color);
    push_vertex(mesh, c, color);
    push_vertex(mesh, d, color);

    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
}

// The corners are given in the winding that faces outwards from the column
static void push_side_face(CloudMesh& mesh, const Eigen::Vector2f& a, const Eigen::Vector2f& b, float y_bottom, float y_top,
    const CloudColor& color)
{
    auto bottom_a = Eigen::Vector3f(a.x(), y_bottom, a.y());
    auto bottom_b = Eigen::Vector3f(b.x(), y_bottom, b.y());
    auto top_b = Eigen::Vector3f(b.x(), y_top, b.y());
    auto top_a = Eigen::Vector3f(a.x(), y_top, a.y());

    push_quad(mesh, bottom_a, bottom_b, top_b, top_a, color);
}

// The vertical spans of `self` that `other` leaves
// exposed. A neighbour can hide a lower and an upper part at
// once, because a column's underside steps down by whole cells
template<typename F>
static void for_exposed_span(const CloudColumn& self, const CloudColumn& other, F&& emit)
{
    if(other.top <= other.bottom) {
        emit(self.bottom, self.top);
        return;
    }

    auto lower_top = std::min(self.top, other.bottom);

    if(lower_top > self.bottom) {
        emit(self.bottom, lower_top);
    }

    auto upper_bottom = std::max(self.bottom, other.top);

    if(upper_bottom < self.top) {
        emit(upper_bottom, self.top);
    }
}

static void push_exposed_sides(CloudMesh& mesh, const CloudColumn& column, const CloudColumn& other, const Eigen::Vector2f& a,
    const Eigen::Vector2f& b, float base_y, float height, const CloudColor& color)
{
    for_exposed_span(column, other, [&](std::int16_t from, std::int16_t to) {
        auto y_bottom = base_y + static_cast<float>(from) * height;
        auto y_top = base_y + static_cast<float>(to) * height;

        push_side_face(mesh, a, b, y_bottom, y_top, color);
    });
}

static CloudDeck make_deck(const CloudInput& input, const CloudGroup& group, const CloudRing& ring)
{
    CloudDeck deck {};
    deck.realm = group.realm;
    deck.type = group.type;
    deck.layer = group.layer;
    deck.cell = ring.cell;
    deck.height = ring.height;
    deck.state = &input.states[static_cast<std::size_t>(group.realm)];
    deck.anchor = input.anchor;
    deck.scale = std::max(group.layer.scale, 8.0f);
    deck.max_levels = std::clamp(static_cast<int>(group.layer.thickness / deck.height), 1, CLOUD_COLUMN_MAX_VOXELS);
    deck.opacity = std::clamp(group.layer.opacity, 0.05f, 1.0f);
    deck.shadow = group.layer.shadow_color;
    deck.lit = group.layer.lit_color;
    deck.side = (deck.shadow + deck.lit) * 0.5f;

    return deck;
}

// A coarse cell stands for the whole patch the finer ring resolves:
// averaging its corners keeps the two rings on the same structure,
// where a point sample would give the coarse ring a pattern the fine one never draws
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

static CloudColumn sample_column(const CloudDeck& deck, std::int32_t cell_x, std::int32_t cell_z)
{
    auto grid_x = deck.cell * (static_cast<float>(cell_x) + 0.5f);
    auto grid_z = deck.cell * (static_cast<float>(cell_z) + 0.5f);

    // The field is sampled in its own frame, so a cell keeps its
    // answer at every rebuild however far the front has drifted.
    // Going through world space and subtracting the drift again
    // would round differently each time and flip cubes near a threshold
    auto field_x = grid_x - deck.anchor.x();
    auto field_z = grid_z - deck.anchor.y();

    auto coverage = column_coverage(deck, field_x, field_z) * deck.layer.coverage;

    if(coverage <= 0.02f) {
        return CloudColumn {};
    }

    // Two coarse octaves: the broad one carries the weight of the deck, the smaller one keeps a thin deck from collapsing
    auto clump = weather::noise_at(field_x / (deck.scale * 2.0f), field_z / (deck.scale * 2.0f), deck.layer.seed + 5);
    auto mass = weather::noise_at(field_x / deck.scale, field_z / deck.scale, deck.layer.seed);
    auto clump_weight = 0.45f + 0.30f * std::clamp(coverage, 0.0f, 1.0f);
    auto density = clump * clump_weight + mass * (1.0f - clump_weight);

    // A thin type keeps only its densest lumps, an overcast keeps almost the whole field
    auto threshold = 0.62f - std::clamp(coverage, 0.0f, 1.0f) * 0.22f;
    auto headroom = density - threshold;

    // The per-cell crumble breaks a thin deck into scraps and frays the rim of a mass, and leaves the inside of a dense deck clean
    auto hash_x = static_cast<std::int32_t>(std::floor(field_x / deck.cell));
    auto hash_z = static_cast<std::int32_t>(std::floor(field_z / deck.cell));
    auto open = 1.0f - std::clamp(coverage, 0.0f, 1.0f);
    auto seam = 1.0f - std::clamp(headroom / 0.18f, 0.0f, 1.0f);
    auto bias = weather::cell_random(hash_x, hash_z, deck.layer.seed + 17) - 0.5f;
    auto value = headroom + bias * 0.5f * std::max(open, seam * 0.4f);

    if(value <= 0.0f) {
        return CloudColumn {};
    }

    // How far past the threshold a cell sits is what makes a lump tall in its middle and thin at its rim
    auto rise = std::clamp(value / 0.30f, 0.0f, 1.0f);
    auto levels = 1 + static_cast<int>(std::lround(rise * static_cast<float>(deck.max_levels - 1)));

    // The underside steps up into the volume here and there, so the deck
    // reads as a volume from below, but never past the layer's own slab: a
    // deck that grew past it would fight the deck above it for the same pixels
    auto step_max = std::min(CLOUD_COLUMN_MAX_STEP, levels - 1);
    auto step = static_cast<int>(std::lround(clump * static_cast<float>(step_max)));

    // The column's own brightness, kept in the field's frame like everything else so a rebuild never repaints the deck
    auto shade = 1.0f + (weather::cell_random(hash_x, hash_z, deck.layer.seed + 41) - 0.5f) * CLOUD_SHADE_VARIATION;

    CloudColumn column {};
    column.bottom = static_cast<std::int16_t>(step);
    column.top = static_cast<std::int16_t>(levels);
    column.shade = shade;

    return column;
}

// A ring leaves the cells inside inner to the finer ring,
// but keeps two of its own cells past that edge: the finer
// ring fades out over that band, so it needs the coarser one already drawn under it
static void fill_columns(const CloudDeck& deck, const CloudWindow& window, const Eigen::Vector3f& center, float inner,
    std::vector<CloudColumn>& columns)
{
    for(std::size_t z = 0; z < window.depth; ++z) {
        for(std::size_t x = 0; x < window.width; ++x) {
            auto cell_x = window.min_x + static_cast<std::int32_t>(x);
            auto cell_z = window.min_z + static_cast<std::int32_t>(z);

            auto cell_center_x = deck.cell * (static_cast<float>(cell_x) + 0.5f);
            auto cell_center_z = deck.cell * (static_cast<float>(cell_z) + 0.5f);

            auto handover = inner - deck.cell * 2.0f;
            auto covered = std::abs(cell_center_x - center.x()) < handover && std::abs(cell_center_z - center.z()) < handover;

            if(inner > 0.0f && covered) {
                continue;
            }

            columns[column_index(window, x, z)] = sample_column(deck, cell_x, cell_z);
        }
    }
}

// The vertices are in world space, so a build that lands after the
// camera moved stays correct; the chunk origin is subtracted at draw time
static void mesh_column(CloudMesh& mesh, const CloudDeck& deck, const CloudWindow& window, const std::vector<CloudColumn>& columns,
    std::size_t x, std::size_t z)
{
    auto column = columns[column_index(window, x, z)];

    if(column.top <= column.bottom) {
        return;
    }

    auto base_y = deck.layer.height;
    auto grid_x = (static_cast<float>(window.min_x + static_cast<std::int32_t>(x)) + 0.5f) * deck.cell;
    auto grid_z = (static_cast<float>(window.min_z + static_cast<std::int32_t>(z)) + 0.5f) * deck.cell;

    auto x0 = grid_x - deck.cell * 0.5f;
    auto x1 = grid_x + deck.cell * 0.5f;
    auto z0 = grid_z - deck.cell * 0.5f;
    auto z1 = grid_z + deck.cell * 0.5f;

    auto y_bottom = base_y + static_cast<float>(column.bottom) * deck.height;
    auto y_top = base_y + static_cast<float>(column.top) * deck.height;

    CloudColor top_color {};
    top_color.rgb = deck.lit * column.shade;
    top_color.alpha = deck.opacity;

    CloudColor bottom_color {};
    bottom_color.rgb = deck.shadow * column.shade;
    bottom_color.alpha = deck.opacity;

    auto top_a = Eigen::Vector3f(x0, y_top, z0);
    auto top_b = Eigen::Vector3f(x0, y_top, z1);
    auto top_c = Eigen::Vector3f(x1, y_top, z1);
    auto top_d = Eigen::Vector3f(x1, y_top, z0);

    auto bottom_a = Eigen::Vector3f(x0, y_bottom, z0);
    auto bottom_b = Eigen::Vector3f(x1, y_bottom, z0);
    auto bottom_c = Eigen::Vector3f(x1, y_bottom, z1);
    auto bottom_d = Eigen::Vector3f(x0, y_bottom, z1);

    // Both caps are meshed; the view culls the one that faces away
    push_quad(mesh, top_a, top_b, top_c, top_d, top_color);
    push_quad(mesh, bottom_a, bottom_b, bottom_c, bottom_d, bottom_color);

    auto column_x = static_cast<std::int32_t>(x);
    auto column_z = static_cast<std::int32_t>(z);

    auto east = column_at(window, columns, column_x + 1, column_z);
    auto west = column_at(window, columns, column_x - 1, column_z);
    auto south = column_at(window, columns, column_x, column_z + 1);
    auto north = column_at(window, columns, column_x, column_z - 1);

    // Every direction gets its own shade, and the column's own tint on top of it, which is what makes the volume read as blocks
    auto side_color = deck.side * column.shade;

    CloudColor east_color {};
    east_color.rgb = side_color;
    east_color.alpha = deck.opacity;

    CloudColor west_color {};
    west_color.rgb = side_color * 0.88f;
    west_color.alpha = deck.opacity;

    CloudColor south_color {};
    south_color.rgb = side_color * 0.95f;
    south_color.alpha = deck.opacity;

    CloudColor north_color {};
    north_color.rgb = side_color * 0.82f;
    north_color.alpha = deck.opacity;

    push_exposed_sides(mesh, column, east, Eigen::Vector2f(x1, z1), Eigen::Vector2f(x1, z0), base_y, deck.height, east_color);
    push_exposed_sides(mesh, column, west, Eigen::Vector2f(x0, z0), Eigen::Vector2f(x0, z1), base_y, deck.height, west_color);
    push_exposed_sides(mesh, column, south, Eigen::Vector2f(x0, z1), Eigen::Vector2f(x1, z1), base_y, deck.height, south_color);
    push_exposed_sides(mesh, column, north, Eigen::Vector2f(x1, z0), Eigen::Vector2f(x0, z0), base_y, deck.height, north_color);
}

static CloudWindow make_window(const Eigen::Vector3f& center, float cell, float radius)
{
    auto min_x = static_cast<std::int32_t>(std::floor((center.x() - radius) / cell));
    auto max_x = static_cast<std::int32_t>(std::floor((center.x() + radius) / cell));
    auto min_z = static_cast<std::int32_t>(std::floor((center.z() - radius) / cell));
    auto max_z = static_cast<std::int32_t>(std::floor((center.z() + radius) / cell));

    CloudWindow window {};
    window.min_x = min_x;
    window.min_z = min_z;
    window.width = static_cast<std::size_t>(max_x - min_x + 1);
    window.depth = static_cast<std::size_t>(max_z - min_z + 1);

    return window;
}

static void mesh_ring(CloudMesh& mesh, const CloudInput& input, const CloudGroup& group, const CloudRing& ring, float inner, float outer,
    float fade)
{
    auto deck = make_deck(input, group, ring);
    auto window = make_window(input.center, deck.cell, outer);
    auto columns = std::vector<CloudColumn>(window.width * window.depth);

    fill_columns(deck, window, input.center, inner, columns);

    auto offset = mesh.indices.size();

    for(std::size_t z = 0; z < window.depth; ++z) {
        for(std::size_t x = 0; x < window.width; ++x) {
            mesh_column(mesh, deck, window, columns, x, z);
        }
    }

    auto count = mesh.indices.size() - offset;

    if(count) {
        // The ring dissolves over the last two cells of the coarser
        // ring into it; the ring at the deck's edge keeps its own wide fade instead
        auto blend_width = (outer < input.radius) ? std::min(ring.cell * 8.0f, outer - inner) : 0.0f;

        CloudBatch batch {};
        batch.offset = offset;
        batch.count = count;
        batch.fade = fade;
        batch.blend_start = outer - blend_width;
        batch.blend_width = blend_width;
        batch.depth_bias = ring.cell * 1.0e-7f;

        mesh.batches.push_back(batch);
    }
}

// One ring of the deck that is actually built: the ring itself and the band it covers
struct CloudSpan final {
    const CloudRing* ring;
    float inner;
    float outer;
};

// The rings tile the range from the camera out to r_cloud_range,
// and they are meshed from the coarsest to the finest: a finer ring
// then draws over the coarse one, so the faded edge blends into it instead of showing the sky
static void mesh_layer(CloudMesh& mesh, const CloudInput& input, const CloudGroup& group)
{
    std::array<CloudSpan, 3> spans {};
    auto span_count = std::size_t { 0 };
    auto inner = 0.0f;

    for(const auto& ring : CLOUD_RINGS) {
        auto outer = std::min(ring.radius, input.radius);

        if(inner >= outer) {
            break;
        }

        spans[span_count] = CloudSpan { &ring, inner, outer };
        span_count += 1;
        inner = outer;
    }

    for(auto i = span_count; i > 0; i -= 1) {
        const auto& span = spans[i - 1];
        auto fade = (span.outer == input.radius) ? span.outer : 0.0f;

        mesh_ring(mesh, input, group, *span.ring, span.inner, span.outer, fade);
    }
}

static void add_group(biome_realm realm, weather_type_id type, const CloudLayer& layer, float camera_y)
{
    if(layer.coverage <= 0.02f) {
        return;
    }

    // A deck belongs to the realm its height sits in; otherwise a type shared by two realms would be built twice in place
    if(utils::realm_of_y(layer.height) != realm) {
        return;
    }

    CloudGroup group {};
    group.realm = realm;
    group.type = type;
    group.layer = layer;
    group.distance = std::abs(layer.height - camera_y);

    s_groups.push_back(group);
}

static void collect_realm_groups(biome_realm realm, float camera_y)
{
    auto bit = 1U << static_cast<unsigned>(realm);

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL || 0 == (def.realms_bits & bit)) {
            continue;
        }

        for(const auto& layer : def.layers) {
            add_group(realm, def.id, layer, camera_y);
        }
    }
}

static void collect_groups(float camera_y)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        collect_realm_groups(static_cast<biome_realm>(i), camera_y);
    }
}

static void upload_mesh(const CloudMesh& mesh, std::size_t slot)
{
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo[slot]);

    if(mesh.vertices.empty()) {
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo[slot]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        return;
    }

    auto vertex_bytes = static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(CloudVertex));
    auto index_bytes = static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t));

    glBufferData(GL_ARRAY_BUFFER, vertex_bytes, mesh.vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo[slot]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_bytes, mesh.indices.data(), GL_DYNAMIC_DRAW);
}

static std::uint64_t state_signature(void)
{
    std::uint64_t hash = 0;

    for(auto& state : weather::states()) {
        hash ^= static_cast<std::uint64_t>((state.types.drift.x() + 4096.0f) / 16.0f);
        hash ^= static_cast<std::uint64_t>((state.types.drift.y() + 4096.0f) / 16.0f) << 1;
        hash ^= static_cast<std::uint64_t>(state.types.scale) << 2;
        hash ^= static_cast<std::uint64_t>(state.types.seed) << 3;
        hash ^= static_cast<std::uint64_t>((state.detail.drift.x() + 4096.0f) / 16.0f) << 4;
        hash ^= static_cast<std::uint64_t>((state.detail.drift.y() + 4096.0f) / 16.0f) << 5;
        hash ^= static_cast<std::uint64_t>(state.detail.scale) << 6;
        hash ^= static_cast<std::uint64_t>(state.detail.seed) << 7;
    }

    return hash;
}

static bool needs_rebuild(void)
{
    if(s_last_cloud_range != camera::cloud_range.value()) {
        return true;
    }

    // The deck is in world space, so leaving a chunk does not invalidate it; only a window that slid far enough does
    if((camera_world_position() - s_last_build_center).norm() > CLOUD_FINE_CELL_BLOCKS * 4.0f) {
        return true;
    }

    // The mesh is only re-evaluated when the field has drifted far
    // enough for the window to slide a cell; the wind offset alone keeps
    // the deck moving in between, and the field is sampled in the
    // same frame the deck lives in, so a rebuild does not reshuffle cubes
    if((s_wind_offset - s_last_build_offset).norm() > CLOUD_FINE_CELL_BLOCKS * 2.0f) {
        return true;
    }

    return s_state_signature != state_signature();
}

// The deck is built in the weather field's own frame and drawn
// with that field's drift, so the cubes keep the shape the field
// gave them while the front travels. Driving the offset from the
// locally integrated wind instead lets the two slowly disagree,
// and the next rebuild then swaps a whole swath of the deck at once
static void advance_wind_offset(const WeatherRealmState& state, biome_realm realm)
{
    if(!s_anchored || s_anchor_realm != realm) {
        s_drift_anchor = state.types.drift - Eigen::Vector2f(s_wind_offset.x(), s_wind_offset.z());
        s_anchor_realm = realm;
        s_anchored = true;
    }

    auto target = Eigen::Vector3f(state.types.drift.x() - s_drift_anchor.x(), 0.0f, state.types.drift.y() - s_drift_anchor.y());

    // A packet that moves the field somewhere else, such as a forced front, must not
    // teleport the deck across the sky; a short ease turns it into a slide. Ordinary
    // drift moves at a couple of blocks per second, so the lag this adds is under a block
    auto step = 1.0f - std::exp(-WIND_EASE_PER_SECOND * std::min(globals::window_frametime, 0.1f));

    s_wind_offset += (target - s_wind_offset) * step;
}

float cloud_renderer::z_near = CLOUD_NEAR_PLANE;
float cloud_renderer::z_far = CLOUD_NEAR_PLANE + CLOUD_DEPTH_MARGIN;

static void update_depth_range(float radius)
{
    cloud_renderer::z_near = CLOUD_NEAR_PLANE;
    cloud_renderer::z_far = radius + CLOUD_DEPTH_MARGIN;
}

// The deck gets a projection of its own: near and far cover its
// own box, so the ground keeps a small range and a precise depth buffer
static Eigen::Matrix4f cloud_view_projection(void)
{
    auto near_plane = cloud_renderer::z_near;
    auto far_plane = cloud_renderer::z_far;
    auto inv_tangent = 1.0f / std::tan(0.5f * utils::radians(camera::vertical_fov.value()));

    Eigen::Matrix4f projection = Eigen::Matrix4f::Zero();
    projection(0, 0) = inv_tangent / globals::aspect;
    projection(1, 1) = inv_tangent;
    projection(2, 2) = -far_plane / (far_plane - near_plane);
    projection(3, 2) = -1.0f;
    projection(2, 3) = -far_plane * near_plane / (far_plane - near_plane);

    return projection * camera::instance.view();
}

// One build on the pool: the snapshot arrives by value, the geometry comes back through the task
class CloudTask final : public Task {
public:
    CloudTask(CloudInput input, const std::vector<CloudGroup>& groups, CloudMesh&& scratch, std::uint64_t generation);
    virtual ~CloudTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    CloudInput m_input;
    std::vector<CloudGroup> m_groups;
    std::uint64_t m_generation;
    CloudMesh m_mesh;
};

CloudTask::CloudTask(CloudInput input, const std::vector<CloudGroup>& groups, CloudMesh&& scratch, std::uint64_t generation)
    : m_input(std::move(input)), m_groups(groups), m_generation(generation), m_mesh(std::move(scratch))
{
    // empty
}

void CloudTask::process(void)
{
    ZoneScopedN("cloud_renderer::process");

    m_mesh.vertices.clear();
    m_mesh.indices.clear();
    m_mesh.batches.clear();
    m_mesh.center = Eigen::Vector2f(m_input.center.x(), m_input.center.z());

    for(const auto& group : m_groups) {
        if(status.load(std::memory_order_relaxed) == task_status::CANCELLED) {
            m_mesh = CloudMesh {};
            return;
        }

        mesh_layer(m_mesh, m_input, group);
    }
}

void CloudTask::finalize(void)
{
    ZoneScopedN("cloud_renderer::finalize");

    s_building = false;

    // A build that started before a reset, or before a newer one, is dropped; the newer one brings the deck up to date
    if(m_generation != s_generation) {
        return;
    }

    // The build lands in the slot that is not on screen, so neither the mesh buffers
    // nor the GL buffers are freed or overwritten while the previous deck is still drawn
    s_front = 1 - s_front;
    s_meshes[s_front] = std::move(m_mesh);

    upload_mesh(s_meshes[s_front], s_front);
}

// Takes the snapshot and hands the build to the pool; the deck keeps drawing the previous mesh until it lands
static void submit_build(void)
{
    auto camera_position = camera_world_position();
    auto radius = std::min(static_cast<float>(constant::CHUNK_SIZE * camera::cloud_range.value()), CLOUD_MAX_RADIUS_BLOCKS);

    update_depth_range(radius);

    CloudInput input {};
    auto states = weather::states();
    std::copy(states.begin(), states.end(), input.states.begin());
    input.anchor = s_drift_anchor;
    input.center = camera_position - s_wind_offset;
    input.radius = radius;

    s_groups.clear();
    collect_groups(camera_position.y());

    // Far decks first: clouds blend, so the order is what makes overlapping layers stack correctly
    std::sort(s_groups.begin(), s_groups.end(), [](const CloudGroup& a, const CloudGroup& b) {
        return a.distance > b.distance;
    });

    s_last_build_offset = s_wind_offset;
    s_last_build_center = camera_position;
    s_last_cloud_range = camera::cloud_range.value();
    s_state_signature = state_signature();
    s_build_cooldown = BUILD_COOLDOWN_SECONDS;

    s_generation += 1;
    s_building = true;

    threading::submit<CloudTask>(std::move(input), s_groups, std::move(s_meshes[1 - s_front]), s_generation);
}

void cloud_renderer::init(void)
{
    update_depth_range(0.0f);

    auto& clouds_binding = cvar::bind_var(s_clouds, "r_clouds");
    clouds_binding.set_help("Draw the cloud deck: 0 = off, 1 = on");
    clouds_binding.set_flags(FCVAR_ARCHIVE);

    s_clouds_gui.bind("r_clouds");
    settings::video.add_child(s_clouds_gui, 5);

    auto cloud_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "cloud");
    auto cloud_ok = s_program.setup(cloud_id);
    vx::throw_if_not_fmt(cloud_ok, "{}: setup failed", cloud_id.full_string());

    su_CloudViewProjection = s_program.add_uniform("u_CloudViewProjection");
    su_CameraOrigin = s_program.add_uniform("u_CameraOrigin");
    su_DeckCenter = s_program.add_uniform("u_DeckCenter");
    su_WindOffset = s_program.add_uniform("u_WindOffset");
    su_CloudTint = s_program.add_uniform("u_CloudTint");
    su_FadeDistance = s_program.add_uniform("u_FadeDistance");
    su_BlendRange = s_program.add_uniform("u_BlendRange");
    su_DepthBias = s_program.add_uniform("u_DepthBias");

    auto stride = static_cast<GLsizei>(sizeof(CloudVertex));

    for(std::size_t i = 0; i < 2; ++i) {
        glGenVertexArrays(1, &s_vao[i]);
        glBindVertexArray(s_vao[i]);

        glGenBuffers(1, &s_vbo[i]);
        glBindBuffer(GL_ARRAY_BUFFER, s_vbo[i]);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(CloudVertex, position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(CloudVertex, color)));
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &s_ibo[i]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo[i]);
    }

    glBindVertexArray(0);
}

void cloud_renderer::shutdown(void)
{
    // A build still in flight must not upload into buffers that are about to go away
    s_generation += 1;
    s_building = false;

    for(std::size_t i = 0; i < 2; ++i) {
        glDeleteBuffers(1, &s_ibo[i]);
        glDeleteBuffers(1, &s_vbo[i]);
        glDeleteVertexArrays(1, &s_vao[i]);
    }

    s_program.destroy();
}

void cloud_renderer::render(void)
{
    ZoneScoped;

    if(!s_clouds.value()) {
        return;
    }

    auto camera_position = camera_world_position();
    auto realm = utils::realm_of_y(camera_position.y());
    auto& state = weather::state(realm);

    advance_wind_offset(state, realm);

    s_build_cooldown = std::max(s_build_cooldown - globals::window_frametime, 0.0f);

    if(needs_rebuild() && !s_building && s_build_cooldown <= 0.0f) {
        submit_build();
    }

    auto& mesh = s_meshes[s_front];

    if(mesh.batches.empty()) {
        return;
    }

    auto cloud_projection = cloud_view_projection();
    auto camera_origin = utils::to_fvec(camera::chunk);
    Eigen::Vector3f cloud_tint = skybox::cloud_tint * fog::daylight;

    vx::throw_if_not(s_program.update());

    glUseProgram(s_program.handle);

    Eigen::Vector2f deck_center = mesh.center + Eigen::Vector2f(s_wind_offset.x(), s_wind_offset.z());

    glUniformMatrix4fv(s_program.uniforms[su_CloudViewProjection].location, 1, GL_FALSE, cloud_projection.data());
    glUniform3fv(s_program.uniforms[su_CameraOrigin].location, 1, camera_origin.data());
    glUniform2fv(s_program.uniforms[su_DeckCenter].location, 1, deck_center.data());
    glUniform3fv(s_program.uniforms[su_WindOffset].location, 1, s_wind_offset.data());
    glUniform3fv(s_program.uniforms[su_CloudTint].location, 1, cloud_tint.data());

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(s_vao[s_front]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo[s_front]);

    for(const auto& batch : mesh.batches) {
        auto offset = reinterpret_cast<const void*>(batch.offset * sizeof(std::uint32_t));

        // Each ring has its own fade; only the one at the deck's edge dissolves
        glUniform1f(s_program.uniforms[su_FadeDistance].location, batch.fade);
        glUniform2f(s_program.uniforms[su_BlendRange].location, batch.blend_start, batch.blend_width);
        glUniform1f(s_program.uniforms[su_DepthBias].location, batch.depth_bias);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.count), GL_UNSIGNED_INT, offset);
    }

    glBindVertexArray(0);

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    globals::num_draw_calls += mesh.batches.size();
    globals::num_draw_vertices += mesh.vertices.size();
}

void cloud_renderer::reset(void)
{
    s_anchored = false;
    s_wind_offset.setZero();
    s_last_build_offset.setZero();
    s_generation += 1;
    s_building = false;
    s_front = 0;

    s_meshes[0] = CloudMesh {};
    s_meshes[1] = CloudMesh {};

    upload_mesh(s_meshes[s_front], s_front);
}
