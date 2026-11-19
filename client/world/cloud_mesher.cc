#include "client/pch.hh"

#include "client/world/cloud_mesher.hh"

static_assert(sizeof(CloudVertex) == 10 * sizeof(float), "CloudVertex must stay packed for the vertex layout");

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

static void push_vertex(CloudMesh& mesh, const Eigen::Vector3f& position, const Eigen::Vector3f& normal, const CloudColor& color)
{
    CloudVertex vertex {};
    vertex.position = position;
    vertex.normal = normal;
    vertex.color = color;

    mesh.vertices.push_back(vertex);
}

static void push_quad(CloudMesh& mesh, const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c,
    const Eigen::Vector3f& d, const Eigen::Vector3f& normal, const CloudColor& color)
{
    auto base = static_cast<std::uint32_t>(mesh.vertices.size());

    push_vertex(mesh, a, normal, color);
    push_vertex(mesh, b, normal, color);
    push_vertex(mesh, c, normal, color);
    push_vertex(mesh, d, normal, color);

    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
}

// The corners are given in the winding that faces outwards from the column
static void push_side_face(CloudMesh& mesh, const Eigen::Vector2f& a, const Eigen::Vector2f& b, float y_bottom, float y_top,
    const Eigen::Vector3f& normal, const CloudColor& color)
{
    auto bottom_a = Eigen::Vector3f(a.x(), y_bottom, a.y());
    auto bottom_b = Eigen::Vector3f(b.x(), y_bottom, b.y());
    auto top_b = Eigen::Vector3f(b.x(), y_top, b.y());
    auto top_a = Eigen::Vector3f(a.x(), y_top, a.y());

    push_quad(mesh, bottom_a, bottom_b, top_b, top_a, normal, color);
}

// The vertical spans of `self` that `other` leaves exposed. A neighbour can
// hide a lower and an upper part at once, because a column's underside steps
// down by whole cells.
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
    const Eigen::Vector2f& b, float base_y, float height, const Eigen::Vector3f& normal, const CloudColor& color)
{
    for_exposed_span(column, other, [&](std::int16_t from, std::int16_t to) {
        auto y_bottom = base_y + static_cast<float>(from) * height;
        auto y_top = base_y + static_cast<float>(to) * height;

        push_side_face(mesh, a, b, y_bottom, y_top, normal, color);
    });
}

// A ring leaves the cells inside inner to the finer ring, but keeps two of its
// own cells past that edge: the finer ring fades out over that band, so it
// needs the coarser one already drawn under it.
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

            columns[column_index(window, x, z)] = cloud_field::sample_column(deck, cell_x, cell_z);
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

    auto base_y = deck.body.height;
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
    push_quad(mesh, top_a, top_b, top_c, top_d, Eigen::Vector3f(0.0f, 1.0f, 0.0f), top_color);
    push_quad(mesh, bottom_a, bottom_b, bottom_c, bottom_d, Eigen::Vector3f(0.0f, -1.0f, 0.0f), bottom_color);

    auto column_x = static_cast<std::int32_t>(x);
    auto column_z = static_cast<std::int32_t>(z);

    auto east = column_at(window, columns, column_x + 1, column_z);
    auto west = column_at(window, columns, column_x - 1, column_z);
    auto south = column_at(window, columns, column_x, column_z + 1);
    auto north = column_at(window, columns, column_x, column_z - 1);

    // The vertex shader lights the sides by their normals, so every side
    // keeps the same albedo and only the column's own tint varies
    CloudColor side_tint {};
    side_tint.rgb = deck.side * column.shade;
    side_tint.alpha = deck.opacity;

    push_exposed_sides(mesh, column, east, Eigen::Vector2f(x1, z1), Eigen::Vector2f(x1, z0), base_y, deck.height,
        Eigen::Vector3f(1.0f, 0.0f, 0.0f), side_tint);
    push_exposed_sides(mesh, column, west, Eigen::Vector2f(x0, z0), Eigen::Vector2f(x0, z1), base_y, deck.height,
        Eigen::Vector3f(-1.0f, 0.0f, 0.0f), side_tint);
    push_exposed_sides(mesh, column, south, Eigen::Vector2f(x0, z1), Eigen::Vector2f(x1, z1), base_y, deck.height,
        Eigen::Vector3f(0.0f, 0.0f, 1.0f), side_tint);
    push_exposed_sides(mesh, column, north, Eigen::Vector2f(x1, z0), Eigen::Vector2f(x0, z0), base_y, deck.height,
        Eigen::Vector3f(0.0f, 0.0f, -1.0f), side_tint);
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
    auto deck = cloud_field::make_deck(input, group, ring);
    auto window = make_window(deck.center, deck.cell, outer);
    auto columns = std::vector<CloudColumn>(window.width * window.depth);

    fill_columns(deck, window, deck.center, inner, columns);

    auto offset = mesh.indices.size();

    for(std::size_t z = 0; z < window.depth; ++z) {
        for(std::size_t x = 0; x < window.width; ++x) {
            mesh_column(mesh, deck, window, columns, x, z);
        }
    }

    auto count = mesh.indices.size() - offset;

    if(count) {
        // The ring dissolves over the last two cells of the coarser ring into
        // it; the ring at the deck's edge keeps its own wide fade instead.
        auto blend_width = (outer < input.radius) ? std::min(ring.cell * 8.0f, outer - inner) : 0.0f;

        CloudBatch batch {};
        batch.offset = offset;
        batch.count = count;
        batch.realm = group.realm;
        batch.center = Eigen::Vector2f(deck.center.x(), deck.center.z());
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

// The rings tile the range from the camera out to r_cloud_range, and they are
// meshed from the coarsest to the finest: a finer ring then draws over the
// coarse one, so the faded edge blends into it instead of showing the sky.
static void mesh_layer(CloudMesh& mesh, const CloudInput& input, const CloudGroup& group)
{
    std::array<CloudSpan, 3> spans {};
    auto span_count = std::size_t { 0 };
    auto inner = 0.0f;

    for(const auto& ring : cloud_field::rings()) {
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

void cloud_mesher::build(const CloudInput& input, std::span<const CloudGroup> groups, CloudMesh& mesh)
{
    mesh.vertices.clear();
    mesh.indices.clear();
    mesh.batches.clear();

    for(const auto& group : groups) {
        mesh_layer(mesh, input, group);
    }
}
