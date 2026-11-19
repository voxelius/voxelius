#include "client/pch.hh"

#include "client/world/lod_chunk_renderer.hh"

#include "core/camera.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/msg/log.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/shader_program.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/lod_manager.hh"
#include "client/world/lod_quad.hh"

constexpr static std::size_t FOG_MODEL = 0;
constexpr static std::uint8_t LEVEL_COUNT = 5;
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

struct LodChunkBatch final {
    std::size_t offset;
    std::size_t count;
    std::uint8_t level;
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

static GLuint s_vao;
static GLuint s_vbo;

static std::vector<LodChunkQuad> s_quads;
static std::vector<LodChunkBatch> s_batches;
static std::uint64_t s_last_revision;
static float s_cooldown;
static bool s_coverage_dirty;

static void on_chunk_mesh(entt::entity entity)
{
    s_coverage_dirty = true;
}

static void on_chunk_ready(const ChunkReadyEvent& event)
{
    s_coverage_dirty = true;
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    s_coverage_dirty = true;
}

// A chunk the client already holds hides its pyramid, mesh or not. An empty
// mesh is still a chunk the player can be inside, and drawing the LOD there
// puts far terrain in the middle of the world.
static bool chunk_is_loaded(const ChunkPos& cpos)
{
    return static_cast<bool>(world::find_chunk(cpos));
}

static bool refresh_coverage(void)
{
    auto changed = false;
    auto& chunks = lod_manager::chunks();

    for(auto& [cpos, entry] : chunks) {
        auto visible = !chunk_is_loaded(cpos);

        if(entry.visible != visible) {
            entry.visible = visible;
            changed = true;
        }
    }

    s_coverage_dirty = false;
    return changed;
}

static void build_batches(void)
{
    auto& chunks = lod_manager::chunks();
    auto realm = utils::realm(camera::chunk.y());
    auto other_realms = lod_manager::other_realms();

    std::array<std::vector<LodChunkQuad>, LEVEL_COUNT> levels;

    for(const auto& [cpos, entry] : chunks) {
        if(!entry.visible || entry.level >= LEVEL_COUNT || entry.instances.empty()) {
            continue;
        }

        if(!other_realms && utils::realm(cpos.y()) != realm) {
            continue;
        }

        auto& bucket = levels[entry.level];
        bucket.insert(bucket.end(), entry.instances.begin(), entry.instances.end());
    }

    s_quads.clear();
    s_batches.clear();

    for(std::uint8_t level = 0; level < LEVEL_COUNT; level += 1) {
        auto& source = levels[level];

        if(source.empty()) {
            continue;
        }

        LodChunkBatch batch {};
        batch.offset = s_quads.size();
        batch.count = source.size();
        batch.level = level;

        s_quads.insert(s_quads.end(), source.begin(), source.end());
        s_batches.push_back(batch);
    }
}

static void upload_batches(void)
{
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(s_quads.size() * sizeof(LodChunkQuad)), s_quads.data(), GL_DYNAMIC_DRAW);
}

void lod_chunk_renderer::init(void)
{
    auto program_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "lod_chunk");
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

    auto stride = static_cast<GLsizei>(sizeof(LodChunkQuad));

    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);

    glGenBuffers(1, &s_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(LodChunkQuad, data_cell)));
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(LodChunkQuad, data_color)));
    glVertexAttribIPointer(2, 3, GL_INT, stride, reinterpret_cast<const void*>(offsetof(LodChunkQuad, origin_x)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    s_quads.clear();
    s_batches.clear();
    s_last_revision = std::numeric_limits<std::uint64_t>::max();
    s_cooldown = 0.0f;
    s_coverage_dirty = true;

    world::chunk_registry.on_update<ChunkMesh>().connect<&on_chunk_mesh>();
    world::chunk_registry.on_destroy<ChunkMesh>().connect<&on_chunk_mesh>();

    globals::dispatcher.sink<ChunkReadyEvent>().connect<&on_chunk_ready>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
}

void lod_chunk_renderer::shutdown(void)
{
    glDeleteBuffers(1, &s_vbo);
    glDeleteVertexArrays(1, &s_vao);

    s_quads.clear();
    s_batches.clear();

    s_program.destroy();
}

void lod_chunk_renderer::prepare(void)
{
    ZoneScoped;

    if(!lod_manager::enabled()) {
        return;
    }

    // Coverage changes with a new packet as well as with a chunk mesh: a
    // pyramid that arrives where a full chunk is already meshed must not be
    // drawn over it, and a pyramid under an unloaded chunk must come back.
    if(s_coverage_dirty || s_last_revision != lod_manager::mesh_revision()) {
        if(refresh_coverage()) {
            lod_manager::notify_mesh_changed();
            s_cooldown = 0.0f;
        }
    }

    s_cooldown = std::max(s_cooldown - globals::window_frametime, 0.0f);

    if(s_cooldown > 0.0f) {
        return;
    }

    auto revision = lod_manager::mesh_revision();

    if(s_last_revision == revision) {
        return;
    }

    build_batches();
    upload_batches();

    s_last_revision = revision;
    s_cooldown = BUILD_COOLDOWN;
}

void lod_chunk_renderer::render(void)
{
    ZoneScoped;

    if(!lod_manager::enabled()) {
        return;
    }

    if(s_batches.empty()) {
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

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    auto stride = static_cast<GLsizei>(sizeof(LodChunkQuad));

    for(const auto& batch : s_batches) {
        auto cell_size = static_cast<float>(1 << batch.level);
        auto depth_bias = -static_cast<float>(LEVEL_COUNT - 1 - batch.level) * 1.0e-7f;
        auto base = static_cast<GLsizeiptr>(batch.offset * sizeof(LodChunkQuad));

        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodChunkQuad, data_cell))));
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodChunkQuad, data_color))));
        glVertexAttribIPointer(2, 3, GL_INT, stride,
            reinterpret_cast<const void*>(base + static_cast<GLsizeiptr>(offsetof(LodChunkQuad, origin_x))));

        glUniform1f(s_program.uniforms[su_CellSize].location, cell_size);
        glUniform1f(s_program.uniforms[su_DepthBias].location, depth_bias);
        glUniform3fv(s_program.uniforms[su_DebugColor].location, 1, DEBUG_LEVEL_COLORS[batch.level].data());
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(batch.count));
    }

    glBindVertexArray(0);

    globals::num_draw_calls += s_batches.size();
    globals::num_draw_vertices += s_quads.size() * 6;
}

void lod_chunk_renderer::log_stats(void)
{
    msg::log_info("lod_chunk: quads={} batches={}", s_quads.size(), s_batches.size());

    for(const auto& batch : s_batches) {
        msg::log_info("lod_chunk: level={} count={}", batch.level, batch.count);
    }
}
