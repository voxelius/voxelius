#include "client/pch.hh"

#include "client/world/lod_tile_renderer.hh"

#include "core/camera.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/msg/log.hh"
#include "core/threading.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"
#include "shared/world/chunk.hh"
#include "shared/world/lod.hh"
#include "shared/world/lod_tile.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/shader_program.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/lod_manager.hh"
#include "client/world/lod_quad.hh"

constexpr static std::size_t FOG_MODEL = 0;
constexpr static float BUILD_COOLDOWN = 0.25f;
constexpr static float AMBIENT_LIGHT = 0.03f;
static const Eigen::Vector3f SKY_LIGHT_COLOR { 1.0f, 1.0f, 1.0f };
static const std::array<Eigen::Vector3f, 9> DEBUG_LEVEL_COLORS = {
    Eigen::Vector3f(1.0f, 0.0f, 0.0f),
    Eigen::Vector3f(0.0f, 1.0f, 0.0f),
    Eigen::Vector3f(0.0f, 0.0f, 1.0f),
    Eigen::Vector3f(1.0f, 1.0f, 0.0f),
    Eigen::Vector3f(1.0f, 0.0f, 1.0f),
    Eigen::Vector3f(0.0f, 1.0f, 1.0f),
    Eigen::Vector3f(1.0f, 0.5f, 0.0f),
    Eigen::Vector3f(0.5f, 0.0f, 1.0f),
    Eigen::Vector3f(1.0f, 1.0f, 1.0f),
};

struct LodTileBatch final {
    std::size_t offset;
    std::size_t count;
    std::uint8_t level;
};

struct LodTileMesh final {
    std::vector<LodTileQuad> quads;
    std::vector<LodTileBatch> batches;
};

struct LodTileBuildCell final {
    bool valid;
    std::int16_t top;
    std::int16_t bottom;
    std::uint32_t color;
};

struct LodTileSnapshot final {
    std::vector<std::pair<LodKey, std::shared_ptr<const LodTile>>> tiles;
    vx::hash_set<LodKey> full_chunk_xz;
    ChunkPos camera;
    unsigned view_distance;
    biome_realm realm;
    bool other_realms;
};

class LodTileBuildTask final : public Task {
public:
    explicit LodTileBuildTask(LodTileSnapshot snapshot, std::uint64_t generation);
    virtual ~LodTileBuildTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    LodTileSnapshot m_snapshot;
    std::uint64_t m_generation;
    LodTileMesh m_mesh;
};

static ShaderProgram s_program;
static std::size_t su_ViewProjection;
static std::size_t su_CameraOrigin;
static std::size_t su_CellSize;
static std::size_t su_DepthBias;
static std::size_t su_FogColor;
static std::size_t su_ViewDistance;
static std::size_t su_SkyColor;
static std::size_t su_Ambient;
static std::size_t su_DebugMode;
static std::size_t su_DebugColor;

static GLuint s_vao[2];
static GLuint s_vbo[2];

static LodTileMesh s_meshes[2];
static std::size_t s_front;
static std::uint64_t s_generation;
static bool s_building;
static float s_cooldown;
static std::uint64_t s_last_revision;
static ChunkPos s_last_camera;
static std::size_t s_last_chunk_count;

static LodKey chunk_realm_key(biome_realm realm, std::int32_t x, std::int32_t z)
{
    LodKey key {};
    key.realm = realm;
    key.level = 0;
    key.x = x;
    key.z = z;
    return key;
}

static bool chunk_is_covered(const vx::hash_set<LodKey>& set, biome_realm realm, std::int64_t block_x, std::int64_t block_z)
{
    auto chunk_x = static_cast<std::int32_t>(block_x >> 4);
    auto chunk_z = static_cast<std::int32_t>(block_z >> 4);
    return set.contains(chunk_realm_key(realm, chunk_x, chunk_z));
}

// A cell can straddle up to four chunks. It is skipped only when all of them
// are covered, so a partially covered edge cell is drawn instead of leaving a
// gap between the tile and the finer geometry.
static bool cell_is_covered(const vx::hash_set<LodKey>& set, biome_realm realm, std::int64_t block_x, std::int64_t block_z,
    std::int64_t cell_size)
{
    auto last = cell_size - 1;

    auto covered = chunk_is_covered(set, realm, block_x, block_z);
    covered = covered && chunk_is_covered(set, realm, block_x + last, block_z);
    covered = covered && chunk_is_covered(set, realm, block_x, block_z + last);
    covered = covered && chunk_is_covered(set, realm, block_x + last, block_z + last);

    return covered;
}

static const LodTileCell* cell_at(const LodTile& tile, std::int32_t x, std::int32_t z)
{
    if(x < 0 || z < 0 || x >= static_cast<std::int32_t>(LOD_TILE_CELLS) || z >= static_cast<std::int32_t>(LOD_TILE_CELLS)) {
        return nullptr;
    }

    return &tile.cells[static_cast<std::size_t>(z * LOD_TILE_CELLS + x)];
}

static std::uint32_t pack_tile_color(const LodTile& tile, const LodTileCell& cell)
{
    auto rgb = lod::to_rgb8(lod::unpack(tile.color_mode, tile.palette, cell.color));
    std::uint32_t color = 0;
    color |= static_cast<std::uint32_t>(rgb[0]);
    color |= static_cast<std::uint32_t>(rgb[1]) << 8U;
    color |= static_cast<std::uint32_t>(rgb[2]) << 16U;

    if(cell.is_water()) {
        color |= LOD_QUAD_WATER_BIT;
    }

    return color;
}

static std::uint32_t pack_tile_height(std::int16_t top, std::int16_t bottom)
{
    auto top_bits = static_cast<std::uint32_t>(static_cast<std::uint16_t>(top));
    auto bottom_bits = static_cast<std::uint32_t>(static_cast<std::uint16_t>(bottom));
    return top_bits | (bottom_bits << 16U);
}

static void push_tile_quad(std::vector<LodTileQuad>& out, std::int32_t origin_x, std::int32_t origin_z, std::int32_t x, std::int32_t z,
    std::uint32_t w, std::uint32_t d, std::uint32_t face, std::int16_t top, std::int16_t bottom, std::uint32_t color)
{
    LodTileQuad quad {};
    auto width = std::min<std::uint32_t>(w, 32U) - 1U;
    auto depth = std::min<std::uint32_t>(d, 32U) - 1U;
    quad.data_cell = 0;
    quad.data_cell |= static_cast<std::uint32_t>(x);
    quad.data_cell |= static_cast<std::uint32_t>(z) << 5U;
    quad.data_cell |= face << 10U;
    quad.data_cell |= width << 13U;
    quad.data_cell |= depth << 18U;
    quad.data_height = pack_tile_height(top, bottom);
    quad.data_color = color;
    quad.origin_x = origin_x;
    quad.origin_z = origin_z;
    out.push_back(quad);
}

static bool cell_can_merge(const std::array<LodTileBuildCell, LOD_TILE_CELL_COUNT>& grid, const std::array<bool, LOD_TILE_CELL_COUNT>& used,
    std::size_t index, const LodTileBuildCell& info, std::int16_t LodTileBuildCell::* edge)
{
    const auto& cell = grid[index];
    return cell.valid && !used[index] && cell.*edge == info.*edge && cell.color == info.color;
}

static bool row_can_merge(const std::array<LodTileBuildCell, LOD_TILE_CELL_COUNT>& grid, const std::array<bool, LOD_TILE_CELL_COUNT>& used,
    std::int32_t z, std::int32_t x, std::int32_t width, const LodTileBuildCell& info, std::int16_t LodTileBuildCell::* edge)
{
    for(auto xx = x; xx < x + width; ++xx) {
        auto index = static_cast<std::size_t>(z * LOD_TILE_CELLS + xx);

        if(!cell_can_merge(grid, used, index, info, edge)) {
            return false;
        }
    }

    return true;
}

static void build_mesh(LodTileMesh& mesh, const LodTileSnapshot& snapshot)
{
    std::array<std::vector<LodTileQuad>, LOD_TILE_LEVEL_MAX + 1> levels;

    mesh.quads.clear();
    mesh.batches.clear();

    auto range = static_cast<std::int64_t>(lod_manager::range_chunks());

    std::vector<std::pair<LodKey, std::shared_ptr<const LodTile>>> ordered = snapshot.tiles;
    std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
        return a.first.level < b.first.level;
    });

    // Finer tiles claim their whole chunk square. A coarser cell is skipped
    // there, so the mesh shows the finest available level per area instead
    // of drawing every level on top of every other one.
    vx::hash_set<LodKey> finer_chunks;

    for(const auto& [key, tile] : ordered) {
        if(!snapshot.other_realms && key.realm != snapshot.realm) {
            continue;
        }

        auto cell_size = lod::tile_cell_size(key.level);
        auto edge_blocks = lod::tile_edge_blocks(key.level);
        auto edge_chunks = lod::tile_edge_chunks(key.level);
        auto origin_x = static_cast<std::int32_t>(static_cast<std::int64_t>(key.x) * edge_blocks);
        auto origin_z = static_cast<std::int32_t>(static_cast<std::int64_t>(key.z) * edge_blocks);

        auto& out = levels[key.level];
        auto emit_bottom = key.realm == BIOME_REALM_SKY;

        std::array<LodTileBuildCell, LOD_TILE_CELL_COUNT> grid {};

        for(std::int32_t z = 0; z < static_cast<std::int32_t>(LOD_TILE_CELLS); ++z) {
            for(std::int32_t x = 0; x < static_cast<std::int32_t>(LOD_TILE_CELLS); ++x) {
                const auto& cell = *cell_at(*tile, x, z);

                if(cell.is_empty()) {
                    continue;
                }

                auto block_x = static_cast<std::int64_t>(key.x) * edge_blocks + static_cast<std::int64_t>(x) * cell_size;
                auto block_z = static_cast<std::int64_t>(key.z) * edge_blocks + static_cast<std::int64_t>(z) * cell_size;

                if(cell_is_covered(snapshot.full_chunk_xz, key.realm, block_x, block_z, cell_size)) {
                    continue;
                }

                auto chunk_x = (block_x + cell_size / 2) >> 4;
                auto chunk_z = (block_z + cell_size / 2) >> 4;

                if(cell_is_covered(finer_chunks, key.realm, block_x, block_z, cell_size)) {
                    continue;
                }

                auto delta_x = chunk_x - snapshot.camera.x();
                auto delta_z = chunk_z - snapshot.camera.z();
                auto distance = std::max(std::abs(delta_x), std::abs(delta_z));
                if(distance > range) {
                    continue;
                }

                auto& info = grid[static_cast<std::size_t>(z * LOD_TILE_CELLS + x)];
                info.valid = true;
                info.top = cell.height;
                info.bottom = cell.bottom;
                info.color = pack_tile_color(*tile, cell);
            }
        }

        std::array<bool, LOD_TILE_CELL_COUNT> used_top {};
        std::array<bool, LOD_TILE_CELL_COUNT> used_bottom {};

        for(std::int32_t z = 0; z < static_cast<std::int32_t>(LOD_TILE_CELLS); ++z) {
            for(std::int32_t x = 0; x < static_cast<std::int32_t>(LOD_TILE_CELLS); ++x) {
                auto index = static_cast<std::size_t>(z * LOD_TILE_CELLS + x);
                const auto& info = grid[index];

                if(!info.valid || used_top[index]) {
                    continue;
                }

                auto w = 1;

                while(x + w < static_cast<std::int32_t>(LOD_TILE_CELLS)) {
                    auto next = static_cast<std::size_t>(z * LOD_TILE_CELLS + x + w);

                    if(!cell_can_merge(grid, used_top, next, info, &LodTileBuildCell::top)) {
                        break;
                    }

                    w += 1;
                }

                auto d = 1;

                while(z + d < static_cast<std::int32_t>(LOD_TILE_CELLS)) {
                    if(!row_can_merge(grid, used_top, z + d, x, w, info, &LodTileBuildCell::top)) {
                        break;
                    }

                    d += 1;
                }

                for(auto zz = z; zz < z + d; zz += 1) {
                    for(auto xx = x; xx < x + w; xx += 1) {
                        used_top[static_cast<std::size_t>(zz * LOD_TILE_CELLS + xx)] = true;
                    }
                }

                push_tile_quad(out, origin_x, origin_z, x, z, static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(d), LOD_FACE_TOP,
                    info.top, info.bottom, info.color);
            }
        }

        if(emit_bottom) {
            for(std::int32_t z = 0; z < static_cast<std::int32_t>(LOD_TILE_CELLS); ++z) {
                for(std::int32_t x = 0; x < static_cast<std::int32_t>(LOD_TILE_CELLS); ++x) {
                    auto index = static_cast<std::size_t>(z * LOD_TILE_CELLS + x);
                    const auto& info = grid[index];

                    if(!info.valid || used_bottom[index]) {
                        continue;
                    }

                    auto w = 1;

                    while(x + w < static_cast<std::int32_t>(LOD_TILE_CELLS)) {
                        auto next = static_cast<std::size_t>(z * LOD_TILE_CELLS + x + w);

                        if(!cell_can_merge(grid, used_bottom, next, info, &LodTileBuildCell::bottom)) {
                            break;
                        }

                        w += 1;
                    }

                    auto d = 1;

                    while(z + d < static_cast<std::int32_t>(LOD_TILE_CELLS)) {
                        if(!row_can_merge(grid, used_bottom, z + d, x, w, info, &LodTileBuildCell::bottom)) {
                            break;
                        }

                        d += 1;
                    }

                    for(auto zz = z; zz < z + d; zz += 1) {
                        for(auto xx = x; xx < x + w; xx += 1) {
                            used_bottom[static_cast<std::size_t>(zz * LOD_TILE_CELLS + xx)] = true;
                        }
                    }

                    push_tile_quad(out, origin_x, origin_z, x, z, static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(d),
                        LOD_FACE_BOTTOM, info.top, info.bottom, info.color);
                }
            }
        }

        for(std::int32_t z = 0; z < static_cast<std::int32_t>(LOD_TILE_CELLS); ++z) {
            for(std::int32_t x = 0; x < static_cast<std::int32_t>(LOD_TILE_CELLS); ++x) {
                const auto& info = grid[static_cast<std::size_t>(z * LOD_TILE_CELLS + x)];

                if(!info.valid) {
                    continue;
                }

                const std::array<std::pair<std::int32_t, std::int32_t>, 4> sides = {
                    std::make_pair(-1, 0),
                    std::make_pair(1, 0),
                    std::make_pair(0, -1),
                    std::make_pair(0, 1),
                };

                for(std::size_t i = 0; i < sides.size(); ++i) {
                    auto nx = x + sides[i].first;
                    auto nz = z + sides[i].second;
                    auto neighbour = cell_at(*tile, nx, nz);
                    auto face = LOD_FACE_NORTH;

                    if(i == 0) {
                        face = LOD_FACE_WEST;
                    }
                    else if(i == 1) {
                        face = LOD_FACE_EAST;
                    }
                    else if(i == 2) {
                        face = LOD_FACE_NORTH;
                    }
                    else {
                        face = LOD_FACE_SOUTH;
                    }

                    if(neighbour && !neighbour->is_empty()) {
                        if(neighbour->height < info.top) {
                            push_tile_quad(out, origin_x, origin_z, x, z, 1, 1, face, info.top, neighbour->height, info.color);
                        }

                        if(neighbour->bottom > info.bottom && emit_bottom) {
                            push_tile_quad(out, origin_x, origin_z, x, z, 1, 1, face, neighbour->bottom, info.bottom, info.color);
                        }

                        continue;
                    }

                    push_tile_quad(out, origin_x, origin_z, x, z, 1, 1, face, info.top, info.bottom, info.color);
                }
            }
        }

        auto base_chunk_x = static_cast<std::int64_t>(key.x) * edge_chunks;
        auto base_chunk_z = static_cast<std::int64_t>(key.z) * edge_chunks;

        for(std::int64_t dz = 0; dz < edge_chunks; dz += 1) {
            for(std::int64_t dx = 0; dx < edge_chunks; dx += 1) {
                finer_chunks.insert(chunk_realm_key(key.realm, static_cast<std::int32_t>(base_chunk_x + dx),
                    static_cast<std::int32_t>(base_chunk_z + dz)));
            }
        }
    }

    for(std::int32_t level = static_cast<std::int32_t>(LOD_TILE_LEVEL_MAX); level >= 0; level -= 1) {
        auto& source = levels[level];

        if(source.empty()) {
            continue;
        }

        LodTileBatch batch {};
        batch.offset = mesh.quads.size();
        batch.count = source.size();
        batch.level = static_cast<std::uint8_t>(level);

        mesh.quads.insert(mesh.quads.end(), source.begin(), source.end());
        mesh.batches.push_back(batch);
    }
}

static void upload_mesh(const LodTileMesh& mesh, std::size_t slot)
{
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo[slot]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.quads.size() * sizeof(LodTileQuad)), mesh.quads.data(), GL_DYNAMIC_DRAW);
}

LodTileBuildTask::LodTileBuildTask(LodTileSnapshot snapshot, std::uint64_t generation)
    : m_snapshot(std::move(snapshot)), m_generation(generation)
{
    // empty
}

void LodTileBuildTask::process(void)
{
    ZoneScopedN("lod_tile_renderer::process");

    build_mesh(m_mesh, m_snapshot);
}

void LodTileBuildTask::finalize(void)
{
    ZoneScopedN("lod_tile_renderer::finalize");

    s_building = false;

    if(m_generation != s_generation) {
        return;
    }

    s_front = 1 - s_front;
    s_meshes[s_front] = std::move(m_mesh);

    upload_mesh(s_meshes[s_front], s_front);
}

static void submit_build(void)
{
    LodTileSnapshot snapshot;
    snapshot.tiles = lod_manager::snapshot_tiles();
    snapshot.camera = camera::chunk;
    snapshot.view_distance = camera::view_distance.value();
    snapshot.realm = utils::realm(camera::chunk.y());
    snapshot.other_realms = lod_manager::other_realms();

    // The tile stays under a chunk that draws nothing: the chunk mesh may be
    // missing its water or not exist at all, and the heightmap is what keeps
    // the sea closed instead of showing the sky.
    for(auto [entity, component, mesh] : world::chunk_registry.view<Chunk_Component, ChunkMesh>().each()) {
        if(mesh.opaque.count == 0 && mesh.alpha.count == 0 && mesh.fluid.count == 0) {
            continue;
        }

        snapshot.full_chunk_xz.insert(chunk_realm_key(utils::realm(component.position.y()), component.position.x(),
            component.position.z()));
    }

    s_last_revision = lod_manager::mesh_revision();
    s_last_camera = camera::chunk;
    s_cooldown = BUILD_COOLDOWN;
    s_building = true;
    s_generation += 1;

    threading::submit<LodTileBuildTask>(std::move(snapshot), s_generation);
}

void lod_tile_renderer::init(void)
{
    auto program_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "lod_tile");
    auto program_ok = s_program.setup(program_id);
    vx::throw_if_not_fmt(program_ok, "{}: setup failed", program_id.full_string());

    su_ViewProjection = s_program.add_uniform("u_ViewProjection");
    su_CameraOrigin = s_program.add_uniform("u_CameraOrigin");
    su_CellSize = s_program.add_uniform("u_CellSize");
    su_DepthBias = s_program.add_uniform("u_DepthBias");
    su_FogColor = s_program.add_uniform("u_FogColor");
    su_ViewDistance = s_program.add_uniform("u_ViewDistance");
    su_SkyColor = s_program.add_uniform("u_SkyColor");
    su_Ambient = s_program.add_uniform("u_Ambient");
    su_DebugMode = s_program.add_uniform("u_DebugMode");
    su_DebugColor = s_program.add_uniform("u_DebugColor");

    auto stride = static_cast<GLsizei>(sizeof(LodTileQuad));

    for(std::size_t i = 0; i < 2; ++i) {
        glGenVertexArrays(1, &s_vao[i]);
        glBindVertexArray(s_vao[i]);

        glGenBuffers(1, &s_vbo[i]);
        glBindBuffer(GL_ARRAY_BUFFER, s_vbo[i]);

        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(LodTileQuad, data_cell)));
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(LodTileQuad, data_height)));
        glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(LodTileQuad, data_color)));
        glVertexAttribIPointer(3, 2, GL_INT, stride, reinterpret_cast<const void*>(offsetof(LodTileQuad, origin_x)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(0, 1);
        glVertexAttribDivisor(1, 1);
        glVertexAttribDivisor(2, 1);
        glVertexAttribDivisor(3, 1);
    }

    glBindVertexArray(0);

    s_front = 0;
    s_generation = 0;
    s_building = false;
    s_cooldown = 0.0f;
    s_last_revision = std::numeric_limits<std::uint64_t>::max();
}

void lod_tile_renderer::shutdown(void)
{
    s_generation += 1;
    s_building = false;

    for(std::size_t i = 0; i < 2; ++i) {
        glDeleteBuffers(1, &s_vbo[i]);
        glDeleteVertexArrays(1, &s_vao[i]);
    }

    s_meshes[0] = LodTileMesh {};
    s_meshes[1] = LodTileMesh {};

    s_program.destroy();
}

void lod_tile_renderer::prepare(void)
{
    ZoneScoped;

    if(!lod_manager::enabled()) {
        return;
    }

    s_cooldown = std::max(s_cooldown - globals::window_frametime, 0.0f);

    if(s_building || s_cooldown > 0.0f) {
        return;
    }

    auto chunk_count = world::chunk_registry.view<ChunkMesh>().size();

    if(s_last_revision != lod_manager::mesh_revision() || !camera::chunk.isApprox(s_last_camera) || s_last_chunk_count != chunk_count) {
        s_last_chunk_count = chunk_count;
        submit_build();
    }
}

void lod_tile_renderer::render(void)
{
    ZoneScoped;

    if(!lod_manager::enabled()) {
        return;
    }

    auto& mesh = s_meshes[s_front];

    if(mesh.batches.empty()) {
        return;
    }

    s_program.set_variant_frag(FOG_MODEL, fog::model.value());
    vx::throw_if_not(s_program.update());

    auto& view_projection = camera::instance.view_projection();
    Eigen::Vector3f camera_origin = utils::to_fvec(camera::chunk);
    auto sky_color = Eigen::Vector3f(SKY_LIGHT_COLOR * fog::daylight * fog::light_scale);

    // Under a fluid the sky light arrives through the fluid, so the far
    // terrain must carry the fluid colour instead of the clear-sky white
    if(camera::inside_fluid) {
        sky_color = fog::color;
    }

    glUseProgram(s_program.handle);
    glUniformMatrix4fv(s_program.uniforms[su_ViewProjection].location, 1, GL_FALSE, view_projection.data());
    glUniform3fv(s_program.uniforms[su_CameraOrigin].location, 1, camera_origin.data());
    glUniform3fv(s_program.uniforms[su_FogColor].location, 1, fog::color.data());
    glUniform1f(s_program.uniforms[su_ViewDistance].location, fog::distance);
    glUniform3fv(s_program.uniforms[su_SkyColor].location, 1, sky_color.data());
    glUniform1f(s_program.uniforms[su_Ambient].location, AMBIENT_LIGHT);
    glUniform1i(s_program.uniforms[su_DebugMode].location, lod_manager::debug_levels() ? 1 : 0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glBindVertexArray(s_vao[s_front]);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo[s_front]);

    auto stride = static_cast<GLsizei>(sizeof(LodTileQuad));

    for(const auto& batch : mesh.batches) {
        auto cell_size = static_cast<float>(1 << batch.level);
        auto depth_bias = -static_cast<float>(LOD_TILE_LEVEL_MAX - batch.level) * 1.0e-7f;
        auto base = static_cast<GLsizeiptr>(batch.offset * sizeof(LodTileQuad));

        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodTileQuad, data_cell))));
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodTileQuad, data_height))));
        glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodTileQuad, data_color))));
        glVertexAttribIPointer(3, 2, GL_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodTileQuad, origin_x))));

        glUniform1f(s_program.uniforms[su_CellSize].location, cell_size);
        glUniform1f(s_program.uniforms[su_DepthBias].location, depth_bias);
        glUniform3fv(s_program.uniforms[su_DebugColor].location, 1, DEBUG_LEVEL_COLORS[batch.level].data());
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(batch.count));
    }

    glBindVertexArray(0);

    globals::num_draw_calls += mesh.batches.size();
    globals::num_draw_vertices += mesh.quads.size() * 6;
}

void lod_tile_renderer::log_stats(void)
{
    auto& mesh = s_meshes[s_front];

    msg::log_info("lod_tile: quads={} batches={}", mesh.quads.size(), mesh.batches.size());

    for(const auto& batch : mesh.batches) {
        msg::log_info("lod_tile: level={} count={}", batch.level, batch.count);
    }
}
