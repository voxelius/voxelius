#include "client/pch.hh"

#include "client/world/cloud_renderer.hh"

#include "core/camera.hh"
#include "core/cvar/host.hh"
#include "core/exception.hh"
#include "core/identifier.hh"

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

constexpr static float CLOUD_VOXEL_WIDTH_BLOCKS = 8.0f; // A longer range adds voxels to the deck instead of growing them
constexpr static float CLOUD_VOXEL_HEIGHT_BLOCKS = 4.0f;
constexpr static int CLOUD_COLUMN_MAX_VOXELS = 8;
constexpr static int CLOUD_COLUMN_MAX_STEP = 2;

constexpr static float CLOUD_SHADE_VARIATION = 0.16f; // Enough to break a deck into blocks, small enough that no cube stands out

// Whatever r_cloud_range says, the deck cannot reach further
// than this: the cost grows with the square of the range,
// so 32 chunks is roughly where a whole deck still builds in a frame
constexpr static float CLOUD_MAX_RADIUS_BLOCKS = static_cast<float>(constant::CHUNK_SIZE * cloud_renderer::MAX_RANGE_CHUNKS);

constexpr static float WIND_EASE_PER_SECOND = 6.0f;

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

// One layer of one weather type, with everything the mesher needs worked out once
struct CloudDeck final {
    biome_realm realm;
    weather_type_id type;
    CloudLayer layer;
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
};

static_assert(sizeof(CloudVertex) == 7 * sizeof(float), "CloudVertex must stay packed for the vertex layout");

static cvar::Number<unsigned> s_clouds { 1, 0, 1 };
static gui::CheckBox s_clouds_gui;

static ShaderProgram s_program;
static std::size_t su_ViewProjection;
static std::size_t su_CameraOrigin;
static std::size_t su_CameraPos;
static std::size_t su_WindOffset;
static std::size_t su_FadeDistance;

static GLuint s_vao;
static GLuint s_vbo;
static GLuint s_ibo;

static std::vector<CloudVertex> s_vertices;
static std::vector<std::uint32_t> s_indices;
static std::vector<CloudBatch> s_batches;
static std::vector<CloudGroup> s_groups;
static Eigen::Vector3f s_wind_offset { Eigen::Vector3f::Zero() };
static Eigen::Vector3f s_last_build_offset { Eigen::Vector3f::Zero() };
static std::uint64_t s_state_signature;
static ChunkPos s_last_camera_chunk { ChunkPos::Zero() };
static unsigned s_last_cloud_range { 0 };
static float s_radius { 0.0f };
static Eigen::Vector2f s_drift_anchor { Eigen::Vector2f::Zero() };
static biome_realm s_anchor_realm { BIOME_REALM_SURFACE };
static bool s_anchored { false };

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

static void push_vertex(const Eigen::Vector3f& position, const CloudColor& color)
{
    CloudVertex vertex {};
    vertex.position = position;
    vertex.color = color;

    s_vertices.push_back(vertex);
}

static void push_quad(const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c, const Eigen::Vector3f& d,
    const CloudColor& color)
{
    auto base = static_cast<std::uint32_t>(s_vertices.size());

    push_vertex(a, color);
    push_vertex(b, color);
    push_vertex(c, color);
    push_vertex(d, color);

    s_indices.push_back(base);
    s_indices.push_back(base + 1);
    s_indices.push_back(base + 2);
    s_indices.push_back(base);
    s_indices.push_back(base + 2);
    s_indices.push_back(base + 3);
}

// The corners are given in the winding that faces outwards from the column
static void push_side_face(const Eigen::Vector2f& a, const Eigen::Vector2f& b, float y_bottom, float y_top, const CloudColor& color)
{
    auto bottom_a = Eigen::Vector3f(a.x(), y_bottom, a.y());
    auto bottom_b = Eigen::Vector3f(b.x(), y_bottom, b.y());
    auto top_b = Eigen::Vector3f(b.x(), y_top, b.y());
    auto top_a = Eigen::Vector3f(a.x(), y_top, a.y());

    push_quad(bottom_a, bottom_b, top_b, top_a, color);
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

static void push_exposed_sides(const CloudColumn& column, const CloudColumn& other, const Eigen::Vector2f& a, const Eigen::Vector2f& b,
    float base_y, const CloudColor& color)
{
    for_exposed_span(column, other, [&](std::int16_t from, std::int16_t to) {
        auto y_bottom = base_y + static_cast<float>(from) * CLOUD_VOXEL_HEIGHT_BLOCKS;
        auto y_top = base_y + static_cast<float>(to) * CLOUD_VOXEL_HEIGHT_BLOCKS;

        push_side_face(a, b, y_bottom, y_top, color);
    });
}

static CloudDeck make_deck(biome_realm realm, weather_type_id type, const CloudLayer& layer)
{
    auto light = fog::daylight.value();

    CloudDeck deck {};
    deck.realm = realm;
    deck.type = type;
    deck.layer = layer;
    deck.scale = std::max(layer.scale, 8.0f);
    deck.max_levels = std::clamp(static_cast<int>(layer.thickness / CLOUD_VOXEL_HEIGHT_BLOCKS), 1, CLOUD_COLUMN_MAX_VOXELS);
    deck.opacity = std::clamp(layer.opacity, 0.05f, 1.0f);
    deck.shadow = layer.shadow_color * light;
    deck.lit = layer.lit_color * light;
    deck.side = (deck.shadow + deck.lit) * 0.5f;

    return deck;
}

static CloudColumn sample_column(const CloudDeck& deck, std::int32_t cell_x, std::int32_t cell_z)
{
    auto grid_x = CLOUD_VOXEL_WIDTH_BLOCKS * (static_cast<float>(cell_x) + 0.5f);
    auto grid_z = CLOUD_VOXEL_WIDTH_BLOCKS * (static_cast<float>(cell_z) + 0.5f);

    // The field is sampled in its own frame, so a cell keeps its
    // answer at every rebuild however far the front has drifted.
    // Going through world space and subtracting the drift again
    // would round differently each time and flip cubes near a threshold
    auto field_x = grid_x - s_drift_anchor.x();
    auto field_z = grid_z - s_drift_anchor.y();

    auto coverage = deck.layer.coverage * weather::coverage_in_field(deck.realm, deck.type, field_x, field_z);

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
    auto hash_x = static_cast<std::int32_t>(std::floor(field_x / CLOUD_VOXEL_WIDTH_BLOCKS));
    auto hash_z = static_cast<std::int32_t>(std::floor(field_z / CLOUD_VOXEL_WIDTH_BLOCKS));
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

static void fill_columns(const CloudDeck& deck, const CloudWindow& window, std::vector<CloudColumn>& columns)
{
    for(std::size_t z = 0; z < window.depth; ++z) {
        for(std::size_t x = 0; x < window.width; ++x) {
            auto cell_x = window.min_x + static_cast<std::int32_t>(x);
            auto cell_z = window.min_z + static_cast<std::int32_t>(z);

            columns[column_index(window, x, z)] = sample_column(deck, cell_x, cell_z);
        }
    }
}

static void mesh_column(const CloudDeck& deck, const CloudWindow& window, const std::vector<CloudColumn>& columns,
    const Eigen::Vector3f& origin, std::size_t x, std::size_t z)
{
    auto column = columns[column_index(window, x, z)];

    if(column.top <= column.bottom) {
        return;
    }

    auto base_y = deck.layer.height - origin.y();
    auto grid_x = (static_cast<float>(window.min_x + static_cast<std::int32_t>(x)) + 0.5f) * CLOUD_VOXEL_WIDTH_BLOCKS - origin.x();
    auto grid_z = (static_cast<float>(window.min_z + static_cast<std::int32_t>(z)) + 0.5f) * CLOUD_VOXEL_WIDTH_BLOCKS - origin.z();

    auto x0 = grid_x - CLOUD_VOXEL_WIDTH_BLOCKS * 0.5f;
    auto x1 = grid_x + CLOUD_VOXEL_WIDTH_BLOCKS * 0.5f;
    auto z0 = grid_z - CLOUD_VOXEL_WIDTH_BLOCKS * 0.5f;
    auto z1 = grid_z + CLOUD_VOXEL_WIDTH_BLOCKS * 0.5f;

    auto y_bottom = base_y + static_cast<float>(column.bottom) * CLOUD_VOXEL_HEIGHT_BLOCKS;
    auto y_top = base_y + static_cast<float>(column.top) * CLOUD_VOXEL_HEIGHT_BLOCKS;

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
    push_quad(top_a, top_b, top_c, top_d, top_color);
    push_quad(bottom_a, bottom_b, bottom_c, bottom_d, bottom_color);

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

    push_exposed_sides(column, east, Eigen::Vector2f(x1, z1), Eigen::Vector2f(x1, z0), base_y, east_color);
    push_exposed_sides(column, west, Eigen::Vector2f(x0, z0), Eigen::Vector2f(x0, z1), base_y, west_color);
    push_exposed_sides(column, south, Eigen::Vector2f(x0, z1), Eigen::Vector2f(x1, z1), base_y, south_color);
    push_exposed_sides(column, north, Eigen::Vector2f(x1, z0), Eigen::Vector2f(x0, z0), base_y, north_color);
}

static void mesh_layer(biome_realm realm, weather_type_id type, const CloudLayer& layer, const CloudWindow& window,
    const Eigen::Vector3f& origin)
{
    auto deck = make_deck(realm, type, layer);
    auto columns = std::vector<CloudColumn>(window.width * window.depth);

    fill_columns(deck, window, columns);

    for(std::size_t z = 0; z < window.depth; ++z) {
        for(std::size_t x = 0; x < window.width; ++x) {
            mesh_column(deck, window, columns, origin, x, z);
        }
    }
}

static CloudWindow make_window(const Eigen::Vector3f& center, float radius)
{
    auto min_x = static_cast<std::int32_t>(std::floor((center.x() - radius) / CLOUD_VOXEL_WIDTH_BLOCKS));
    auto max_x = static_cast<std::int32_t>(std::floor((center.x() + radius) / CLOUD_VOXEL_WIDTH_BLOCKS));
    auto min_z = static_cast<std::int32_t>(std::floor((center.z() - radius) / CLOUD_VOXEL_WIDTH_BLOCKS));
    auto max_z = static_cast<std::int32_t>(std::floor((center.z() + radius) / CLOUD_VOXEL_WIDTH_BLOCKS));

    CloudWindow window {};
    window.min_x = min_x;
    window.min_z = min_z;
    window.width = static_cast<std::size_t>(max_x - min_x + 1);
    window.depth = static_cast<std::size_t>(max_z - min_z + 1);

    return window;
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

static void upload_mesh(void)
{
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    if(s_vertices.empty()) {
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        return;
    }

    auto vertex_bytes = static_cast<GLsizeiptr>(s_vertices.size() * sizeof(CloudVertex));
    auto index_bytes = static_cast<GLsizeiptr>(s_indices.size() * sizeof(std::uint32_t));

    glBufferData(GL_ARRAY_BUFFER, vertex_bytes, s_vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_bytes, s_indices.data(), GL_DYNAMIC_DRAW);
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

    if(!camera::chunk.isApprox(s_last_camera_chunk)) {
        return true;
    }

    // The mesh is only re-evaluated when the field has drifted far
    // enough for the window to slide a cell; the wind offset alone keeps
    // the deck moving in between, and the field is sampled in the
    // same frame the deck lives in, so a rebuild does not reshuffle cubes
    if((s_wind_offset - s_last_build_offset).norm() > CLOUD_VOXEL_WIDTH_BLOCKS * 2.0f) {
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

static void rebuild(void)
{
    s_vertices.clear();
    s_indices.clear();
    s_batches.clear();
    s_groups.clear();

    auto camera_position = camera_world_position();
    auto origin = utils::to_fvec(camera::chunk);

    auto radius = std::min(static_cast<float>(constant::CHUNK_SIZE * camera::cloud_range.value()), CLOUD_MAX_RADIUS_BLOCKS);
    s_radius = radius;

    auto center = camera_position - s_wind_offset;
    auto window = make_window(center, radius);

    collect_groups(camera_position.y());

    // Far decks first: clouds blend, so the order is what makes overlapping layers stack correctly
    std::sort(s_groups.begin(), s_groups.end(), [](const CloudGroup& a, const CloudGroup& b) {
        return a.distance > b.distance;
    });

    for(const auto& group : s_groups) {
        auto offset = s_indices.size();

        mesh_layer(group.realm, group.type, group.layer, window, origin);

        auto count = s_indices.size() - offset;

        if(count) {
            CloudBatch batch {};
            batch.offset = offset;
            batch.count = count;

            s_batches.push_back(batch);
        }
    }

    upload_mesh();
}

void cloud_renderer::init(void)
{
    auto& clouds_binding = cvar::bind_var(s_clouds, "r_clouds");
    clouds_binding.set_help("Draw the cloud deck: 0 = off, 1 = on");
    clouds_binding.set_flags(FCVAR_ARCHIVE);

    s_clouds_gui.bind("r_clouds");
    settings::video.add_child(s_clouds_gui, 5);

    auto cloud_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "cloud");
    auto cloud_ok = s_program.setup(cloud_id);
    vx::throw_if_not_fmt(cloud_ok, "{}: setup failed", cloud_id.full_string());

    su_ViewProjection = s_program.add_uniform("u_ViewProjection");
    su_CameraOrigin = s_program.add_uniform("u_CameraOrigin");
    su_CameraPos = s_program.add_uniform("u_CameraPos");
    su_WindOffset = s_program.add_uniform("u_WindOffset");
    su_FadeDistance = s_program.add_uniform("u_FadeDistance");

    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);

    glGenBuffers(1, &s_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    auto stride = static_cast<GLsizei>(sizeof(CloudVertex));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(CloudVertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(CloudVertex, color)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &s_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);

    glBindVertexArray(0);
}

void cloud_renderer::shutdown(void)
{
    glDeleteBuffers(1, &s_ibo);
    glDeleteBuffers(1, &s_vbo);
    glDeleteVertexArrays(1, &s_vao);

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

    if(needs_rebuild()) {
        s_state_signature = state_signature();
        s_last_camera_chunk = camera::chunk;
        s_last_cloud_range = camera::cloud_range.value();

        rebuild();

        s_last_build_offset = s_wind_offset;
    }

    if(s_batches.empty()) {
        return;
    }

    auto& vproj = camera::instance.view_projection();
    auto camera_origin = utils::to_fvec(camera::chunk);

    vx::throw_if_not(s_program.update());

    glUseProgram(s_program.handle);

    glUniformMatrix4fv(s_program.uniforms[su_ViewProjection].location, 1, GL_FALSE, vproj.data());
    glUniform3fv(s_program.uniforms[su_CameraOrigin].location, 1, camera_origin.data());
    glUniform3fv(s_program.uniforms[su_CameraPos].location, 1, camera_position.data());
    glUniform3fv(s_program.uniforms[su_WindOffset].location, 1, s_wind_offset.data());
    glUniform1f(s_program.uniforms[su_FadeDistance].location, s_radius);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);

    for(const auto& batch : s_batches) {
        auto offset = reinterpret_cast<const void*>(batch.offset * sizeof(std::uint32_t));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.count), GL_UNSIGNED_INT, offset);
    }

    glBindVertexArray(0);

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    globals::num_draw_calls += s_batches.size();
    globals::num_draw_vertices += s_vertices.size();
}

void cloud_renderer::reset(void)
{
    // The anchor belongs to the field the deck was built against; a fresh session brings a fresh field, so both are dropped
    s_anchored = false;
    s_wind_offset.setZero();
    s_last_build_offset.setZero();
}
