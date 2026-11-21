#include "client/pch.hh"

#include "client/world/chunk_mesher.hh"

#include "core/threading.hh"

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/chunk.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/light_material.hh"
#include "shared/world/tint_registry.hh"
#include "shared/world/world.hh"

#include "client/world/biome_cache.hh"
#include "client/world/block_atlas.hh"
#include "client/world/block_cache.hh"
#include "client/world/bmodel_cache.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/chunk_vbo.hh"
#include "client/world/fluid_cache.hh"

#include "client/camera.hh"
#include "client/globals.hh"

constexpr static std::size_t MAX_SUBMIT_PER_FRAME = 64;

// How far ahead of the player the mesh order is biased, in chunks. The loader
// fetches that way, so the mesher follows, or the leading edge would still
// wait behind the ground already flown over.
constexpr static float LOOKAHEAD_CHUNKS = 4.0f;

constexpr static std::array ALL_FACES = {
    BLOCK_FACE_NORTH,
    BLOCK_FACE_SOUTH,
    BLOCK_FACE_EAST,
    BLOCK_FACE_WEST,
    BLOCK_FACE_TOP,
    BLOCK_FACE_BOTTOM,
};

class MeshingTask final : public Task {
public:
    explicit MeshingTask(entt::entity entity, const ChunkPos& cpos);
    virtual ~MeshingTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    std::uint32_t calculate_ao(const LocalPos& lpos, block_face face, const Eigen::Vector3f& vertex) const;
    block_light_type sample_light(const LocalPos& lpos, block_face face, const Eigen::Vector3f& vertex, light_channel channel) const;

    bool is_culled_block(const LocalPos& lpos, block_face face, block_render self_render) const;
    bool is_culled_fluid(const LocalPos& lpos, block_face face, fluid_id_type fluid_id, bool face_flush) const;
    bool is_occluder(const LocalPos& lpos, block_face exposed_face) const;

    void emit_block_quads(std::vector<ChunkMesh_Vertex>& out, std::span<const CachedBlockModel_Quad> quads, const LocalPos& lpos,
        std::uint64_t entropy, std::optional<block_face> face) const;
    void emit_fluid_face(std::vector<ChunkMesh_Vertex>& out, const LocalPos& lpos, const std::array<Eigen::Vector3f, 4>& positions,
        block_face face, const AtlasStrip* strip, tint_id_type tint, std::uint32_t mask_frame,
        std::optional<std::array<Eigen::Vector2f, 4>> uvs_override = std::nullopt) const;

    Eigen::Vector3f resolve_tint(const LocalPos& lpos, tint_id_type tint) const;

    void mesh_fluid(const LocalPos& lpos, const BlockDefinition& def);
    void mesh_block(const LocalPos& lpos, block_id_type id);

    void reset_tint_cache(void);

    BlockCache m_cache;
    BiomeCache m_biomes;

    mutable std::array<std::pair<tint_id_type, Eigen::Vector3f>, 8> m_tint_cache;
    mutable std::size_t m_tint_cache_size { 0 };

    std::vector<ChunkMesh_Vertex> m_opaque;
    std::vector<ChunkMesh_Vertex> m_alpha;
    std::vector<ChunkMesh_Vertex> m_fluid;
    entt::entity m_entity;
    ChunkPos m_cpos;
};

struct CornerSamples final {
    LocalPos side1;
    LocalPos side2;
    LocalPos corner;
};

static vx::hash_set<ChunkPos> s_pending;
static std::size_t s_backlog;

constexpr static block_face opposite_face(block_face face)
{
    switch(face) {
        case BLOCK_FACE_NORTH:
            return BLOCK_FACE_SOUTH;

        case BLOCK_FACE_SOUTH:
            return BLOCK_FACE_NORTH;

        case BLOCK_FACE_EAST:
            return BLOCK_FACE_WEST;

        case BLOCK_FACE_WEST:
            return BLOCK_FACE_EAST;

        case BLOCK_FACE_TOP:
            return BLOCK_FACE_BOTTOM;

        case BLOCK_FACE_BOTTOM:
            return BLOCK_FACE_TOP;
    }

    return BLOCK_FACE_NORTH;
}

static ChunkPos face_delta(block_face face)
{
    switch(face) {
        case BLOCK_FACE_NORTH:
            return -ChunkPos::UnitZ();

        case BLOCK_FACE_SOUTH:
            return ChunkPos::UnitZ();

        case BLOCK_FACE_EAST:
            return ChunkPos::UnitX();

        case BLOCK_FACE_WEST:
            return -ChunkPos::UnitX();

        case BLOCK_FACE_TOP:
            return ChunkPos::UnitY();

        case BLOCK_FACE_BOTTOM:
            return -ChunkPos::UnitY();
    }

    return ChunkPos::Zero();
}

static float mesh_queue_distance_sq(const ChunkPos& cpos, const Eigen::Vector3f& focus)
{
    Eigen::Vector3f position = utils::to_fvec(cpos - camera::chunk) * static_cast<float>(constant::CHUNK_SIZE);
    Eigen::Vector3f delta = position - camera::local - focus;
    return delta.squaredNorm();
}

static float shade_factor(const Eigen::Vector3f& normal)
{
    auto ax = std::abs(normal.x());
    auto ay = std::abs(normal.y());
    auto az = std::abs(normal.z());

    if(ay >= ax && ay >= az) {
        if(normal.y()) {
            return 1.0f;
        }
        else {
            return 0.4f;
        }
    }

    if(ax >= az) {
        return 0.6f;
    }
    else {
        return 0.8f;
    }
}

static CornerSamples sample_corner(const LocalPos& lpos, block_face face, const Eigen::Vector3f& vertex)
{
    auto delta = face_delta(face);
    LocalPos::value_type nx = lpos.x() + delta.x();
    LocalPos::value_type ny = lpos.y() + delta.y();
    LocalPos::value_type nz = lpos.z() + delta.z();

    LocalPos::value_type dx = vertex.x() > 0.5f ? 1 : -1;
    LocalPos::value_type dy = vertex.y() > 0.5f ? 1 : -1;
    LocalPos::value_type dz = vertex.z() > 0.5f ? 1 : -1;

    auto s1x = nx;
    auto s1y = ny;
    auto s1z = nz;

    auto s2x = nx;
    auto s2y = ny;
    auto s2z = nz;

    if(delta.x()) {
        s1y += dy;
        s2z += dz;
    }
    else if(delta.y()) {
        s1x += dx;
        s2z += dz;
    }
    else {
        s1x += dx;
        s2y += dy;
    }

    CornerSamples samples;
    samples.side1 = LocalPos(s1x, s1y, s1z);
    samples.side2 = LocalPos(s2x, s2y, s2z);
    samples.corner = LocalPos(s1x + s2x - nx, s1y + s2y - ny, s1z + s2z - nz);
    return samples;
}

static void emit_quad_vertices(std::vector<ChunkMesh_Vertex>& out, const std::array<Eigen::Vector3f, 4>& positions_16ths,
    const std::array<Eigen::Vector2f, 4>& uvs, const std::array<std::uint32_t, 4>& ao_values, std::uint32_t data_3, float shade,
    bool animated, bool flip_quad, const Eigen::Vector3f& tint_rgb, const std::array<block_light_type, 4>& sky_light,
    const std::array<block_light_type, 4>& block_light, std::uint32_t mask_frame)
{
    std::array<int, 4> order;

    if(flip_quad) {
        order[0] = 0;
        order[1] = 1;
        order[2] = 3;
        order[3] = 2;
    }
    else {
        order[0] = 1;
        order[1] = 2;
        order[2] = 0;
        order[3] = 3;
    }

    for(auto corner : order) {
        ChunkMesh_Vertex vertex {};
        vertex.data_1 = ChunkMesh_Vertex::pack_1(positions_16ths[corner]);
        vertex.data_2 = ChunkMesh_Vertex::pack_2(uvs[corner], mask_frame);
        vertex.data_3 = data_3;
        vertex.data_4 = ChunkMesh_Vertex::pack_4(ao_values[corner], shade, animated, tint_rgb, sky_light[corner], block_light[corner]);
        out.push_back(vertex);
    }
}

static void sync_part(ChunkMesh_Part& part, std::uint32_t slot)
{
    auto old_count = part.count;
    auto old_base = part.base_vertex;

    if(part.vertices.empty()) {
        if(old_count > 0) {
            chunk_vbo::free(old_base, old_count);
        }

        part.count = 0;
        part.base_vertex = 0;
        return;
    }

    for(auto& vertex : part.vertices) {
        vertex.data_5 = slot;
    }

    if(old_count > 0) {
        chunk_vbo::free(old_base, old_count);
    }

    part.count = static_cast<std::uint32_t>(part.vertices.size());
    part.base_vertex = chunk_vbo::allocate(part.count);
    chunk_vbo::upload(part.base_vertex, part.vertices);
}

MeshingTask::MeshingTask(entt::entity entity, const ChunkPos& cpos) : m_entity(entity), m_cpos(cpos)
{
    m_cache.init(cpos);
    m_biomes.init(cpos);
}

void MeshingTask::process(void)
{
    ZoneScopedN("chunk_mesher::process");

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        if(status.load(std::memory_order_relaxed) == task_status::CANCELLED) {
            m_opaque.clear();
            m_alpha.clear();
            m_fluid.clear();
            return;
        }

        auto lpos = utils::to_local(i);
        mesh_block(lpos, m_cache.get(lpos));
    }
}

void MeshingTask::finalize(void)
{
    ZoneScopedN("chunk_mesher::finalize");

    auto& chunk_registry = globals::world->chunk_registry();

    if(!chunk_registry.valid(m_entity) || !chunk_registry.all_of<Chunk_Component>(m_entity)) {
        s_pending.erase(m_cpos);
        return;
    }

    const auto& chunk = chunk_registry.get<Chunk_Component>(m_entity);

    if(chunk.position != m_cpos) {
        s_pending.erase(m_cpos);
        return;
    }

    if(m_opaque.empty() && m_alpha.empty() && m_fluid.empty()) {
        chunk_registry.remove<ChunkMesh>(m_entity);
        s_pending.erase(m_cpos);
        return;
    }

    auto& component = chunk_registry.get_or_emplace<ChunkMesh>(m_entity);

    component.opaque.vertices = std::move(m_opaque);
    sync_part(component.opaque, component.slot);

    component.alpha.vertices = std::move(m_alpha);
    sync_part(component.alpha, component.slot);

    component.fluid.vertices = std::move(m_fluid);
    sync_part(component.fluid, component.slot);

    // Opaque vertices won't get depth-sorted
    // at runtime so there's no point in keeping
    // them persistent in the system memory
    component.opaque.vertices.clear();
    component.opaque.vertices.shrink_to_fit();

    chunk_registry.patch<ChunkMesh>(m_entity);

    s_pending.erase(m_cpos);
}

std::uint32_t MeshingTask::calculate_ao(const LocalPos& lpos, block_face face, const Eigen::Vector3f& vertex) const
{
    auto samples = sample_corner(lpos, face, vertex);

    auto oc_s1 = is_occluder(samples.side1, face);
    auto oc_s2 = is_occluder(samples.side2, face);
    auto oc_c = is_occluder(samples.corner, face);

    if(oc_s1 && oc_s2)
        return 0;
    return 3 - (oc_s1 + oc_s2 + oc_c);
}

block_light_type MeshingTask::sample_light(const LocalPos& lpos, block_face face, const Eigen::Vector3f& vertex,
    light_channel channel) const
{
    auto samples = sample_corner(lpos, face, vertex);
    LocalPos face_cell = lpos + face_delta(face);

    // A cell that holds an opaque block carries no light of its own, and the
    // block need not cover the whole cell: a slab leaves half of it open.
    // Average the light of the cells around the vertex, skipping the ones a
    // fully opaque block fills, instead of taking the single dark cell in
    // front.
    const std::array<LocalPos, 4> cells = { face_cell, samples.side1, samples.side2, samples.corner };

    auto sum = 0U;
    auto count = 0U;

    for(const auto& cell : cells) {
        if(light_material::is_opaque(m_cache.get(cell))) {
            continue;
        }

        sum += m_cache.get_light(cell, channel);
        count += 1;
    }

    if(count == 0) {
        return m_cache.get_light(face_cell, channel);
    }

    return static_cast<block_light_type>(sum / count);
}

bool MeshingTask::is_culled_block(const LocalPos& lpos, block_face face, block_render self_render) const
{
    auto neighbour_lpos = lpos + face_delta(face);
    auto neighbour_id = m_cache.get(neighbour_lpos);

    if(neighbour_id == BLOCK_ID_NULL) {
        return false;
    }

    auto neighbour_baked = bmodel_cache::find(neighbour_id);
    auto neighbour_def = block_registry::find_definition(neighbour_id);

    if(neighbour_baked == nullptr || neighbour_def == nullptr) {
        return false;
    }

    if(neighbour_def->render != self_render) {
        // Differing render modes shouldn't cull
        // each other; this way stuff like glass
        // panes don't cull ground beneath them
        return false;
    }

    return neighbour_baked->fully_covered[opposite_face(face)];
}

bool MeshingTask::is_culled_fluid(const LocalPos& lpos, block_face face, fluid_id_type fluid_id, bool face_flush) const
{
    auto neighbour_lpos = lpos + face_delta(face);
    auto neighbour_id = m_cache.get(neighbour_lpos);

    if(neighbour_id == BLOCK_ID_NULL) {
        return false;
    }

    auto neighbour_def = block_registry::find_definition(neighbour_id);

    if(neighbour_def == nullptr) {
        return false;
    }

    if(neighbour_def->fluid == fluid_id && neighbour_def->fluid_level > 0) {
        return face == BLOCK_FACE_TOP || face == BLOCK_FACE_BOTTOM;
    }

    if(face_flush) {
        if(neighbour_def->render != BLOCK_RENDER_SOLID) {
            return false;
        }

        auto neighbour_baked = bmodel_cache::find(neighbour_id);

        if(neighbour_baked == nullptr) {
            return false;
        }

        return neighbour_baked->fully_covered[opposite_face(face)];
    }

    return false;
}

bool MeshingTask::is_occluder(const LocalPos& lpos, block_face exposed_face) const
{
    auto id = m_cache.get(lpos);

    if(id == BLOCK_ID_NULL) {
        return false;
    }

    auto def = block_registry::find_definition(id);

    if(def == nullptr || def->render != BLOCK_RENDER_SOLID) {
        return false;
    }

    auto baked = bmodel_cache::find(id);

    if(baked == nullptr) {
        return false;
    }

    return baked->fully_covered[opposite_face(exposed_face)];
}

void MeshingTask::emit_block_quads(std::vector<ChunkMesh_Vertex>& out, std::span<const CachedBlockModel_Quad> quads, const LocalPos& lpos,
    std::uint64_t entropy, std::optional<block_face> face) const
{
    auto block_origin = lpos.cast<float>() * 16.0f;

    for(const auto& quad : quads) {
        std::uint32_t frame_offset = 0;

        if(quad.albedo_frames > 0 && !quad.animated) {
            frame_offset = static_cast<std::uint32_t>(entropy % quad.albedo_frames);
        }

        std::array<Eigen::Vector3f, 4> positions_16ths;
        positions_16ths[0] = block_origin + 16.0f * quad.positions[0];
        positions_16ths[1] = block_origin + 16.0f * quad.positions[1];
        positions_16ths[2] = block_origin + 16.0f * quad.positions[2];
        positions_16ths[3] = block_origin + 16.0f * quad.positions[3];

        std::array<std::uint32_t, 4> ao_values;
        ao_values.fill(3);

        if(face.has_value()) {
            for(int i = 0; i < 4; ++i) {
                ao_values[i] = calculate_ao(lpos, face.value(), quad.positions[i]);
            }
        }

        // An unculled quad can sit inside the cell, such as the cut face of
        // a slab, so its light is sampled on the side its normal points into
        // rather than off the block itself.
        auto light_face = BLOCK_FACE_NORTH;

        if(face.has_value()) {
            light_face = face.value();
        }
        else {
            light_face = bmodel_cache::face_from_normal(quad.packed_normal);
        }

        std::array<block_light_type, 4> sky_light;
        std::array<block_light_type, 4> block_light;

        for(int i = 0; i < 4; ++i) {
            sky_light[i] = sample_light(lpos, light_face, quad.positions[i], LIGHT_SKY);
            block_light[i] = sample_light(lpos, light_face, quad.positions[i], LIGHT_BLOCK);
        }

        auto shade = 1.0f;

        if(quad.shade) {
            auto edge_u = positions_16ths[1] - positions_16ths[0];
            auto edge_v = positions_16ths[3] - positions_16ths[0];
            shade = shade_factor(edge_u.cross(edge_v).normalized());
        }

        auto tint_rgb = resolve_tint(lpos, quad.tint);
        auto data_3 = ChunkMesh_Vertex::pack_3(quad.albedo_strip, frame_offset);

        if(ao_values[0] + ao_values[2] < ao_values[1] + ao_values[3]) {
            emit_quad_vertices(out, positions_16ths, quad.uvs, ao_values, data_3, shade, quad.animated, true, tint_rgb, sky_light,
                block_light, quad.mask_frame);
        }
        else {
            emit_quad_vertices(out, positions_16ths, quad.uvs, ao_values, data_3, shade, quad.animated, false, tint_rgb, sky_light,
                block_light, quad.mask_frame);
        }
    }
}

Eigen::Vector3f MeshingTask::resolve_tint(const LocalPos& lpos, tint_id_type tint) const
{
    if(tint == TINT_ID_NULL) {
        return Eigen::Vector3f::Ones();
    }

    for(std::size_t i = 0; i < m_tint_cache_size; ++i) {
        if(m_tint_cache[i].first == tint) {
            return m_tint_cache[i].second;
        }
    }

    Eigen::Vector3f sum = Eigen::Vector3f::Zero();

    for(int dz = -1; dz <= 1; dz += 1) {
        for(int dx = -1; dx <= 1; dx += 1) {
            auto biome_id = m_biomes.get(LocalPosXZ(lpos.x() + dx, lpos.z() + dz));
            auto biome_def = biome_registry::find_definition(biome_id);

            if(biome_def && tint < biome_def->tint_colors.size()) {
                sum += biome_def->tint_colors[tint];
                continue;
            }

            if(auto tint_def = tint_registry::find_definition(tint)) {
                sum += tint_def->default_color;
                continue;
            }

            sum += Eigen::Vector3f::Ones();
        }
    }

    auto result = sum / 9.0f;

    if(m_tint_cache_size < m_tint_cache.size()) {
        m_tint_cache[m_tint_cache_size++] = std::make_pair(tint, result);
    }

    return result;
}

static float fluid_surface_height(const BlockCache& cache, const LocalPos& lpos, const BlockDefinition* def, fluid_gravity gravity,
    unsigned full_level)
{
    auto height = 0.0625f * static_cast<float>(def->fluid_level);

    if(height <= 0.0f) {
        return 0.0f;
    }

    block_face anti_gravity;

    if(gravity == FLUID_GRAVITY_DOWN) {
        anti_gravity = BLOCK_FACE_TOP;
    }
    else {
        anti_gravity = BLOCK_FACE_BOTTOM;
    }

    auto above_id = cache.get(lpos + face_delta(anti_gravity));
    auto above_def = block_registry::find_definition(above_id);

    if(above_def && above_def->fluid == def->fluid && above_def->fluid_level > 0) {
        return 1.0f;
    }

    if(def->fluid_level >= full_level && above_def && above_def->render == BLOCK_RENDER_SOLID) {
        auto above_baked = bmodel_cache::find(above_id);
        auto face = opposite_face(anti_gravity);

        if(above_baked && above_baked->fully_covered[face]) {
            return 1.0f;
        }
    }

    return height;
}

static std::optional<float> fluid_cell_height(const BlockCache& cache, const LocalPos& lpos, fluid_id_type fluid_id, fluid_gravity gravity,
    unsigned full_level)
{
    auto def = block_registry::find_definition(cache.get(lpos));

    if(def == nullptr || def->fluid != fluid_id || def->fluid_level == 0) {
        return std::nullopt;
    }

    return fluid_surface_height(cache, lpos, def, gravity, full_level);
}

static float fluid_corner_height(const BlockCache& cache, const LocalPos& lpos, fluid_id_type fluid_id, fluid_gravity gravity, int sx,
    int sz, float self_height, unsigned full_level)
{
    auto sum = 0.0f;
    auto weight_sum = 0.0f;
    auto full_height = 0.0625f * static_cast<float>(full_level);

    for(int dx = sx - 1; dx <= sx; dx += 1) {
        for(int dz = sz - 1; dz <= sz; dz += 1) {
            auto height = fluid_cell_height(cache, lpos + LocalPos(dx, 0, dz), fluid_id, gravity, full_level);

            if(!height.has_value()) {
                continue;
            }

            if(height.value() >= 1.0f) {
                return 1.0f;
            }

            if(height.value() >= full_height) {
                sum += 10.0f * height.value();
                weight_sum += 10.0f;
            }
            else {
                sum += height.value();
                weight_sum += 1.0f;
            }
        }
    }

    if(weight_sum == 0.0f) {
        return self_height;
    }

    return sum / weight_sum;
}

static std::array<float, 4> fluid_corner_heights(const BlockCache& cache, const LocalPos& lpos, fluid_id_type fluid_id,
    fluid_gravity gravity, float self_height, unsigned full_level)
{
    return {
        fluid_corner_height(cache, lpos, fluid_id, gravity, 0, 0, self_height, full_level),
        fluid_corner_height(cache, lpos, fluid_id, gravity, 0, 1, self_height, full_level),
        fluid_corner_height(cache, lpos, fluid_id, gravity, 1, 1, self_height, full_level),
        fluid_corner_height(cache, lpos, fluid_id, gravity, 1, 0, self_height, full_level),
    };
}

static float fluid_surface_y(float corner_height, fluid_gravity gravity)
{
    if(gravity == FLUID_GRAVITY_DOWN) {
        return corner_height;
    }

    return 1.0f - corner_height;
}

static float fluid_floor_y(fluid_gravity gravity)
{
    if(gravity == FLUID_GRAVITY_DOWN) {
        return 0.0f;
    }

    return 1.0f;
}

static std::optional<float> fluid_neighbour_level(const BlockCache& cache, const LocalPos& lpos, fluid_id_type fluid_id)
{
    auto def = block_registry::find_definition(cache.get(lpos));

    if(def == nullptr || def->fluid != fluid_id || def->fluid_level == 0) {
        return std::nullopt;
    }

    return 0.0625f * static_cast<float>(def->fluid_level);
}

static std::array<Eigen::Vector2f, 4> flowing_top_uvs(float flow_x, float flow_z)
{
    std::array<Eigen::Vector2f, 4> uvs = {
        Eigen::Vector2f(0.0f, 0.0f),
        Eigen::Vector2f(0.0f, 1.0f),
        Eigen::Vector2f(1.0f, 1.0f),
        Eigen::Vector2f(1.0f, 0.0f),
    };

    if(flow_x * flow_x + flow_z * flow_z <= 1.0e-8f) {
        return uvs;
    }

    auto angle = std::atan2(flow_x, flow_z);
    auto quarter = static_cast<int>(std::lround(angle / (0.5f * std::numbers::pi_v<float>)));
    quarter %= 4;
    quarter += 4;
    quarter %= 4;

    switch(quarter) {
        case 1:
            uvs[0] = Eigen::Vector2f(0.0f, 0.0f);
            uvs[1] = Eigen::Vector2f(1.0f, 0.0f);
            uvs[2] = Eigen::Vector2f(1.0f, 1.0f);
            uvs[3] = Eigen::Vector2f(0.0f, 1.0f);
            break;

        case 2:
            uvs[0] = Eigen::Vector2f(1.0f, 1.0f);
            uvs[1] = Eigen::Vector2f(1.0f, 0.0f);
            uvs[2] = Eigen::Vector2f(0.0f, 0.0f);
            uvs[3] = Eigen::Vector2f(0.0f, 1.0f);
            break;

        case 3:
            uvs[0] = Eigen::Vector2f(0.0f, 1.0f);
            uvs[1] = Eigen::Vector2f(1.0f, 1.0f);
            uvs[2] = Eigen::Vector2f(1.0f, 0.0f);
            uvs[3] = Eigen::Vector2f(0.0f, 0.0f);
            break;
    }

    return uvs;
}

void MeshingTask::emit_fluid_face(std::vector<ChunkMesh_Vertex>& out, const LocalPos& lpos, const std::array<Eigen::Vector3f, 4>& positions,
    block_face face, const AtlasStrip* strip, tint_id_type tint, std::uint32_t mask_frame,
    std::optional<std::array<Eigen::Vector2f, 4>> uvs_override) const
{
    if(strip == nullptr) {
        return;
    }

    std::array<Eigen::Vector2f, 4> uvs {};

    if(uvs_override.has_value()) {
        uvs = uvs_override.value();
    }
    else {
        switch(face) {
            case BLOCK_FACE_TOP:
                uvs[0] = Eigen::Vector2f(0.0f, 0.0f);
                uvs[1] = Eigen::Vector2f(0.0f, 1.0f);
                uvs[2] = Eigen::Vector2f(1.0f, 1.0f);
                uvs[3] = Eigen::Vector2f(1.0f, 0.0f);
                break;

            case BLOCK_FACE_BOTTOM:
                uvs[0] = Eigen::Vector2f(1.0f, 1.0f);
                uvs[1] = Eigen::Vector2f(1.0f, 0.0f);
                uvs[2] = Eigen::Vector2f(0.0f, 0.0f);
                uvs[3] = Eigen::Vector2f(0.0f, 1.0f);
                break;

            default:
                uvs[0] = Eigen::Vector2f(1.0f, 1.0f - positions[0].y());
                uvs[1] = Eigen::Vector2f(0.0f, 1.0f - positions[1].y());
                uvs[2] = Eigen::Vector2f(0.0f, 1.0f - positions[2].y());
                uvs[3] = Eigen::Vector2f(1.0f, 1.0f - positions[3].y());
                break;
        }
    }

    auto block_origin = 16.0f * lpos.cast<float>();

    std::array<Eigen::Vector3f, 4> positions_16ths;
    positions_16ths[0] = block_origin + 16.0f * positions[0];
    positions_16ths[1] = block_origin + 16.0f * positions[1];
    positions_16ths[2] = block_origin + 16.0f * positions[2];
    positions_16ths[3] = block_origin + 16.0f * positions[3];

    std::array<std::uint32_t, 4> ao_values;
    ao_values.fill(3);

    auto strip_index = static_cast<std::uint32_t>(strip->index);
    auto data_3 = ChunkMesh_Vertex::pack_3(strip_index, 0);
    auto tint_rgb = resolve_tint(lpos, tint);

    auto light_pos = lpos + face_delta(face);

    std::array<block_light_type, 4> sky_light;
    std::array<block_light_type, 4> block_light;

    sky_light.fill(m_cache.get_light(light_pos, LIGHT_SKY));
    block_light.fill(m_cache.get_light(light_pos, LIGHT_BLOCK));

    if(positions[0].y() + positions[2].y() < positions[1].y() + positions[3].y()) {
        emit_quad_vertices(out, positions_16ths, uvs, ao_values, data_3, 1.0f, true, true, tint_rgb, sky_light, block_light, mask_frame);
    }
    else {
        emit_quad_vertices(out, positions_16ths, uvs, ao_values, data_3, 1.0f, true, false, tint_rgb, sky_light, block_light, mask_frame);
    }
}

void MeshingTask::mesh_fluid(const LocalPos& lpos, const BlockDefinition& def)
{
    auto fluid = fluid_registry::find_definition(def.fluid);
    auto cached = fluid_cache::find(def.fluid);

    if(fluid == nullptr || cached == nullptr) {
        return;
    }

    auto height = 0.0625f * static_cast<float>(def.fluid_level);

    if(height <= 0.0f) {
        return;
    }

    auto surface_height = fluid_surface_height(m_cache, lpos, &def, fluid->gravity, fluid->full_level);
    auto corners = fluid_corner_heights(m_cache, lpos, def.fluid, fluid->gravity, surface_height, fluid->full_level);
    auto& bucket = fluid->opaque ? m_opaque : m_fluid;
    auto tint = fluid->tint;

    auto use_still = def.fluid_level >= fluid->full_level;
    auto flow_x = 0.0f;
    auto flow_z = 0.0f;

    if(!use_still) {
        const std::array dirs = {
            LocalPos(-1, 0, 0),
            LocalPos(+1, 0, 0),
            LocalPos(0, 0, -1),
            LocalPos(0, 0, +1),
        };

        for(const auto& dir : dirs) {
            auto neighbour_h = fluid_neighbour_level(m_cache, lpos + dir, def.fluid);

            if(!neighbour_h.has_value()) {
                continue;
            }

            auto delta = height - neighbour_h.value();
            flow_x += delta * static_cast<float>(dir.x());
            flow_z += delta * static_cast<float>(dir.z());
        }
    }

    auto floor_y = fluid_floor_y(fluid->gravity);
    auto y0 = fluid_surface_y(corners[0], fluid->gravity);
    auto y1 = fluid_surface_y(corners[1], fluid->gravity);
    auto y2 = fluid_surface_y(corners[2], fluid->gravity);
    auto y3 = fluid_surface_y(corners[3], fluid->gravity);

    for(auto face : ALL_FACES) {
        auto is_side = false;
        is_side = is_side || face == BLOCK_FACE_NORTH;
        is_side = is_side || face == BLOCK_FACE_SOUTH;
        is_side = is_side || face == BLOCK_FACE_EAST;
        is_side = is_side || face == BLOCK_FACE_WEST;

        if(is_side) {
            auto neighbour_lpos = lpos + face_delta(face);
            auto neighbour_id = m_cache.get(neighbour_lpos);
            auto neighbour_def = block_registry::find_definition(neighbour_id);

            if(neighbour_def && neighbour_def->fluid == def.fluid && neighbour_def->fluid_level > 0) {
                continue;
            }
        }

        auto surface_face = !is_side;
        auto face_flush = true;

        if(fluid->gravity == FLUID_GRAVITY_DOWN) {
            surface_face = surface_face && face == BLOCK_FACE_TOP;
        }
        else {
            surface_face = surface_face && face == BLOCK_FACE_BOTTOM;
        }

        if(surface_face) {
            face_flush = true;

            for(float corner : corners) {
                if(corner < 1.0f) {
                    face_flush = false;
                    break;
                }
            }
        }

        if(is_culled_fluid(lpos, face, def.fluid, face_flush)) {
            continue;
        }

        std::array<Eigen::Vector3f, 4> positions {};

        switch(face) {
            case BLOCK_FACE_NORTH:
                positions[0] = Eigen::Vector3f(0.0f, y0, 0.0f);
                positions[1] = Eigen::Vector3f(1.0f, y3, 0.0f);
                positions[2] = Eigen::Vector3f(1.0f, floor_y, 0.0f);
                positions[3] = Eigen::Vector3f(0.0f, floor_y, 0.0f);
                break;

            case BLOCK_FACE_SOUTH:
                positions[0] = Eigen::Vector3f(1.0f, y2, 1.0f);
                positions[1] = Eigen::Vector3f(0.0f, y1, 1.0f);
                positions[2] = Eigen::Vector3f(0.0f, floor_y, 1.0f);
                positions[3] = Eigen::Vector3f(1.0f, floor_y, 1.0f);
                break;

            case BLOCK_FACE_EAST:
                positions[0] = Eigen::Vector3f(1.0f, y3, 0.0f);
                positions[1] = Eigen::Vector3f(1.0f, y2, 1.0f);
                positions[2] = Eigen::Vector3f(1.0f, floor_y, 1.0f);
                positions[3] = Eigen::Vector3f(1.0f, floor_y, 0.0f);
                break;

            case BLOCK_FACE_WEST:
                positions[0] = Eigen::Vector3f(0.0f, y1, 1.0f);
                positions[1] = Eigen::Vector3f(0.0f, y0, 0.0f);
                positions[2] = Eigen::Vector3f(0.0f, floor_y, 0.0f);
                positions[3] = Eigen::Vector3f(0.0f, floor_y, 1.0f);
                break;

            case BLOCK_FACE_TOP:
                if(fluid->gravity == FLUID_GRAVITY_DOWN) {
                    positions[0] = Eigen::Vector3f(0.0f, y0, 0.0f);
                    positions[1] = Eigen::Vector3f(0.0f, y1, 1.0f);
                    positions[2] = Eigen::Vector3f(1.0f, y2, 1.0f);
                    positions[3] = Eigen::Vector3f(1.0f, y3, 0.0f);
                }
                else {
                    positions[0] = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
                    positions[1] = Eigen::Vector3f(0.0f, 1.0f, 1.0f);
                    positions[2] = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
                    positions[3] = Eigen::Vector3f(1.0f, 1.0f, 0.0f);
                }
                break;

            case BLOCK_FACE_BOTTOM:
                if(fluid->gravity == FLUID_GRAVITY_UP) {
                    positions[0] = Eigen::Vector3f(0.0f, y0, 1.0f);
                    positions[1] = Eigen::Vector3f(0.0f, y1, 0.0f);
                    positions[2] = Eigen::Vector3f(1.0f, y2, 0.0f);
                    positions[3] = Eigen::Vector3f(1.0f, y3, 1.0f);
                }
                else {
                    positions[0] = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
                    positions[1] = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
                    positions[2] = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
                    positions[3] = Eigen::Vector3f(1.0f, 0.0f, 1.0f);
                }
                break;
        }

        if(is_side) {
            auto h0 = std::abs(positions[0].y() - positions[3].y());
            auto h1 = std::abs(positions[1].y() - positions[2].y());

            if(h0 <= 1.0e-4f && h1 <= 1.0e-4f) {
                continue;
            }

            auto mask = block_atlas::stub_white;

            if(fluid->mask_flowing.has_value()) {
                mask = cached->mask_flowing ? cached->mask_flowing : block_atlas::stub_white;
            }

            emit_fluid_face(bucket, lpos, positions, face, cached->albedo_flowing, tint, static_cast<std::uint32_t>(mask->frame_base));
        }
        else if(surface_face) {
            const AtlasStrip* strip;

            if(use_still) {
                strip = cached->albedo_still;
            }
            else if(cached->albedo_flowing) {
                strip = cached->albedo_flowing;
            }
            else {
                strip = cached->albedo_still;
            }

            std::optional<std::array<Eigen::Vector2f, 4>> uvs;

            if(!use_still && cached->albedo_flowing) {
                uvs = flowing_top_uvs(flow_x, flow_z);
            }

            auto mask = block_atlas::stub_white;
            auto& fluid_mask = use_still ? fluid->mask_still : fluid->mask_flowing;
            auto cached_mask = use_still ? cached->mask_still : cached->mask_flowing;

            if(fluid_mask.has_value()) {
                mask = cached_mask ? cached_mask : block_atlas::stub_white;
            }

            emit_fluid_face(bucket, lpos, positions, face, strip, tint, static_cast<std::uint32_t>(mask->frame_base), uvs);
        }
        else {
            auto mask = block_atlas::stub_white;

            if(fluid->mask_still.has_value()) {
                mask = cached->mask_still ? cached->mask_still : block_atlas::stub_white;
            }

            emit_fluid_face(bucket, lpos, positions, face, cached->albedo_still, tint, static_cast<std::uint32_t>(mask->frame_base));
        }
    }
}

void MeshingTask::mesh_block(const LocalPos& lpos, block_id_type id)
{
    if(id == BLOCK_ID_NULL) {
        return;
    }

    reset_tint_cache();

    auto def = block_registry::find_definition(id);

    if(def == nullptr) {
        return;
    }

    auto baked = bmodel_cache::find(id);

    if(baked && def->render) {
        std::array<bool, 6> culled {};
        auto fully_enclosed = true;

        for(auto face : ALL_FACES) {
            culled[face] = is_culled_block(lpos, face, def->render);
            fully_enclosed = fully_enclosed && culled[face];
        }

        std::uint64_t entropy;

        auto bpos = utils::to_block(m_cpos, lpos);
        entropy = static_cast<std::uint64_t>(bpos.x()) * UINT64_C(0x9E3779B97F4A7C15);
        entropy ^= static_cast<std::uint64_t>(bpos.y()) * UINT64_C(0xC2B2AE3D27D4EB4F);
        entropy ^= static_cast<std::uint64_t>(bpos.z()) * UINT64_C(0x165667B19E3779F9);

        // Just a simple-ish hash for a single 64-bit value; before that
        // we were using CRC64 which is kind of an overkill for a mesher
        // https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html
        entropy ^= entropy >> 33;
        entropy *= UINT64_C(0xFF51AFD7ED558CCD);
        entropy ^= entropy >> 33;
        entropy *= UINT64_C(0xC4CEB9FE1A85EC53);
        entropy ^= entropy >> 33;

        if(def->render == BLOCK_RENDER_ALPHA) {
            if(!fully_enclosed) {
                emit_block_quads(m_alpha, baked->unculled_quads, lpos, entropy, std::nullopt);
            }

            for(auto face : ALL_FACES) {
                if(culled[face])
                    continue;
                emit_block_quads(m_alpha, baked->face_quads[face], lpos, entropy, face);
            }
        }
        else {
            if(!fully_enclosed) {
                emit_block_quads(m_opaque, baked->unculled_quads, lpos, entropy, std::nullopt);
            }

            for(auto face : ALL_FACES) {
                if(culled[face])
                    continue;
                emit_block_quads(m_opaque, baked->face_quads[face], lpos, entropy, face);
            }
        }
    }

    if(def->fluid) {
        mesh_fluid(lpos, *def);
    }
}

void MeshingTask::reset_tint_cache(void)
{
    m_tint_cache_size = 0;
}

static void mark_dirty(entt::entity entity)
{
    auto& chunk_registry = globals::world->chunk_registry();
    chunk_registry.emplace_or_replace<ChunkMesh_DirtyMarker>(entity);
}

static void mark_dirty(const ChunkPos& cpos)
{
    if(auto chunk = globals::world->find_chunk(cpos)) {
        mark_dirty(chunk->entity());
    }
}

static bool touches_neighbor_chunk(const LocalPos& lpos, ChunkPos::value_type dx, ChunkPos::value_type dy, ChunkPos::value_type dz)
{
    constexpr auto size = static_cast<LocalPos::value_type>(constant::CHUNK_SIZE);

    if(dx < 0 && lpos.x() > 0) {
        return false;
    }

    if(dx > 0 && lpos.x() < size - 1) {
        return false;
    }

    if(dy < 0 && lpos.y() > 0) {
        return false;
    }

    if(dy > 0 && lpos.y() < size - 1) {
        return false;
    }

    if(dz < 0 && lpos.z() > 0) {
        return false;
    }

    if(dz > 0 && lpos.z() < size - 1) {
        return false;
    }

    return true;
}

static void mark_neighbors_dirty(const ChunkPos& cpos)
{
    for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
        for(ChunkPos::value_type dy = -1; dy <= 1; dy += 1) {
            for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
                if(dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                mark_dirty(cpos + ChunkPos(dx, dy, dz));
            }
        }
    }
}

static void mark_border_neighbors_dirty(const ChunkPos& cpos, const LocalPos& lpos)
{
    for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
        for(ChunkPos::value_type dy = -1; dy <= 1; dy += 1) {
            for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
                if(dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                if(!touches_neighbor_chunk(lpos, dx, dy, dz)) {
                    continue;
                }

                mark_dirty(cpos + ChunkPos(dx, dy, dz));
            }
        }
    }
}

static void on_chunk_create(const ChunkCreateEvent& event)
{
    auto chunk = event.chunk();
    mark_dirty(chunk->entity());
    mark_neighbors_dirty(event.pos());
}

static void on_chunk_update(const ChunkUpdateEvent& event)
{
    auto chunk = event.chunk();
    mark_dirty(chunk->entity());
    mark_neighbors_dirty(event.pos());
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    auto chunk = event.chunk();
    auto entity = chunk->entity();

    auto& chunk_registry = globals::world->chunk_registry();

    if(chunk_registry.valid(entity)) {
        chunk_registry.remove<ChunkMesh_DirtyMarker>(entity);
        chunk_registry.remove<ChunkMesh>(entity);
    }
}

static void on_block_update(const BlockUpdateEvent& event)
{
    auto chunk = event.chunk();
    mark_dirty(chunk->entity());
    mark_border_neighbors_dirty(event.cpos(), event.lpos());
}

static void on_chunk_light(const ChunkLightUpdateEvent& event)
{
    // A vertex samples the light of every cell around it, diagonals
    // included, so any of the 26 neighbours can need its mesh rebuilt.
    mark_dirty(event.pos());
    mark_neighbors_dirty(event.pos());
}

void chunk_mesher::init(void)
{
    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkUpdateEvent>().connect<&on_chunk_update>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
    globals::dispatcher.sink<ChunkLightUpdateEvent>().connect<&on_chunk_light>();
}

void chunk_mesher::reset(void)
{
    s_pending.clear();
}

void chunk_mesher::update(void)
{
    static std::vector<std::tuple<entt::entity, ChunkPos, float>> batch;

    ZoneScoped;

    TracyPlot("Mesh queue", static_cast<int64_t>(s_pending.size()));

    auto& chunk_registry = globals::world->chunk_registry();
    auto group = chunk_registry.group<ChunkMesh_DirtyMarker>(entt::get<Chunk_Component>);

    batch.clear();

    auto focus = camera::lookahead(LOOKAHEAD_CHUNKS) * static_cast<float>(constant::CHUNK_SIZE);

    for(const auto [entity, chunk] : group.each()) {
        if(0 == s_pending.count(chunk.position)) {
            batch.emplace_back(entity, chunk.position, mesh_queue_distance_sq(chunk.position, focus));
        }
    }

    s_backlog = batch.size() + s_pending.size();

    auto submit_count = std::min(batch.size(), MAX_SUBMIT_PER_FRAME);

    std::partial_sort(batch.begin(), batch.begin() + submit_count, batch.end(), [](const auto& a, const auto& b) {
        const auto& [a_entity, a_position, a_dist_sq] = a;
        const auto& [b_entity, b_position, b_dist_sq] = b;
        return a_dist_sq < b_dist_sq;
    });

    for(std::size_t i = 0; i < submit_count; ++i) {
        const auto& [entity, position, dist_sq] = batch[i];
        s_pending.emplace(position);
        chunk_registry.remove<ChunkMesh_DirtyMarker>(entity);
        threading::submit<MeshingTask>(entity, position);
    }
}

std::size_t chunk_mesher::backlog(void)
{
    return s_backlog;
}
