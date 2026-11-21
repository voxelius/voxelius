#include "client/pch.hh"

#include "client/world/chunk_renderer.hh"

#include "core/camera.hh"
#include "core/cvar/number.hh"
#include "core/exception.hh"
#include "core/frustum.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/utils/chunk.hh"
#include "shared/utils/coord.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/shader_program.hh"
#include "client/world/block_atlas.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/chunk_vbo.hh"

// ONLY TOUCH THESE IF THE RESPECTIVE SHADER
// VARIANT MACRO DECLARATIONS LAYOUT CHANGED AS WELL
constexpr static unsigned FOG_MODEL = 0;

// This value "won" the test of drawing 6080892 vertices in a single
// frame for a decent hardware (cryptomined-to-shit RX 590); I tested multiple
// values and the framerate extrema appears to sit around 1024:
//  BATCH_MAX_PARTS     AVG. FPS
//   512                84.792
//  1024                92.376
//  1280                89.142  <-- looks like a cache-miss
//  2048                89.510
//  4096                77.742
constexpr static std::size_t BATCH_MAX_PARTS = 1024;

constexpr static float AMBIENT_LIGHT = 0.03f;

static const Eigen::Vector3f SKY_LIGHT_COLOR { 1.0f, 1.0f, 1.0f };
static const Eigen::Vector3f BLOCK_LIGHT_COLOR { 1.0f, 0.86f, 0.66f };

static ShaderProgram s_program;
static std::size_t su_ViewProjection;
static std::size_t su_AnimationTimer;
static std::size_t su_ViewDistance;
static std::size_t su_FogColor;
static std::size_t su_SkyColor;
static std::size_t su_BlockColor;
static std::size_t su_Ambient;
static std::size_t su_CameraLocal;
static std::size_t su_AtlasTexture;
static std::size_t su_AtlasStrips;
static std::size_t su_AtlasFrames;
static std::size_t su_ChunkPositions;

static GLuint s_vao;
static GLuint s_pattern_ibo;
static GLuint s_chunk_positions_buffer;
static GLuint s_chunk_positions_tbo;

static std::vector<Eigen::Vector3f> s_chunk_positions;
static std::size_t s_chunk_positions_capacity;

static std::vector<GLsizei> s_multidraw_counts;
static std::vector<GLint> s_multidraw_basevertices;
static std::vector<const void*> s_multidraw_indices;

static bool s_sorted_dirty;
static std::vector<std::pair<entt::entity, float>> s_sorted_chunks;

static ChunkPos s_last_camera_chunk;

static std::size_t s_batch_draw_index;
static std::uint32_t s_batch_vertex_count;

static std::size_t s_pattern_ibo_capacity;
static std::size_t s_pattern_ibo_size;
static std::vector<GLintptr> s_pattern_offsets;

static bool s_frame_ready;

static void ensure_multidraw_capacity(std::size_t draw_count)
{
    if(s_multidraw_counts.size() < draw_count) {
        s_multidraw_counts.resize(draw_count);
        s_multidraw_basevertices.resize(draw_count);
        s_multidraw_indices.resize(draw_count);
    }
}

static void ensure_pattern_ibo_capacity(std::size_t required)
{
    static std::vector<std::byte> scratch;

    auto new_capacity = s_pattern_ibo_capacity;

    while(new_capacity < required) {
        new_capacity <<= 1;
    }

    if(new_capacity == s_pattern_ibo_capacity) {
        return;
    }

    scratch.resize(s_pattern_ibo_size);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_pattern_ibo);
    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(scratch.size()), scratch.data());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(new_capacity), nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(scratch.size()), scratch.data());

    s_pattern_ibo_capacity = new_capacity;
}

static GLintptr pattern_offset(std::uint32_t quad_count)
{
    if(quad_count == 0) {
        return 0;
    }

    if(quad_count >= s_pattern_offsets.size()) {
        s_pattern_offsets.resize(quad_count + 1, -1);
    }

    if(s_pattern_offsets[quad_count] >= 0) {
        return s_pattern_offsets[quad_count];
    }

    std::vector<std::uint32_t> indices;
    indices.resize(6 * static_cast<std::size_t>(quad_count));

    for(std::uint32_t quad = 0; quad < quad_count; quad += 1) {
        auto vertex = quad * 4U;

        indices[static_cast<std::size_t>(quad) * 6 + 0] = vertex + 0;
        indices[static_cast<std::size_t>(quad) * 6 + 1] = vertex + 1;
        indices[static_cast<std::size_t>(quad) * 6 + 2] = vertex + 2;

        indices[static_cast<std::size_t>(quad) * 6 + 3] = vertex + 2;
        indices[static_cast<std::size_t>(quad) * 6 + 4] = vertex + 1;
        indices[static_cast<std::size_t>(quad) * 6 + 5] = vertex + 3;
    }

    auto byte_offset = static_cast<GLintptr>(s_pattern_ibo_size);
    auto byte_size = static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t));

    ensure_pattern_ibo_capacity(s_pattern_ibo_size + static_cast<std::size_t>(byte_size));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_pattern_ibo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, byte_offset, byte_size, indices.data());

    s_pattern_ibo_size += static_cast<std::size_t>(byte_size);
    s_pattern_offsets[quad_count] = byte_offset;
    return byte_offset;
}

static void ensure_chunk_positions_capacity(std::uint32_t slot_count)
{
    if(slot_count <= s_chunk_positions_capacity) {
        return;
    }

    auto new_capacity = s_chunk_positions_capacity;

    while(new_capacity < slot_count) {
        new_capacity <<= 1;
    }

    s_chunk_positions.resize(new_capacity, Eigen::Vector3f::Zero());

    glBindBuffer(GL_TEXTURE_BUFFER, s_chunk_positions_buffer);
    glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(new_capacity * sizeof(Eigen::Vector3f)), nullptr, GL_DYNAMIC_DRAW);

    glBindTexture(GL_TEXTURE_BUFFER, s_chunk_positions_tbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, s_chunk_positions_buffer);

    s_chunk_positions_capacity = new_capacity;
}

static void update_chunk_positions_tbo(void)
{
    std::uint32_t max_slot = 0;

    auto& chunk_registry = globals::world->chunk_registry();
    auto view = chunk_registry.view<Chunk_Component, ChunkMesh>();

    for(const auto [entity, chunk, mesh] : view.each()) {
        if(mesh.slot == std::numeric_limits<std::uint32_t>::max()) {
            continue; // invalid slot
        }

        max_slot = std::max(max_slot, mesh.slot + 1);
    }

    if(max_slot == 0) {
        return;
    }

    ensure_chunk_positions_capacity(max_slot);

    for(const auto [entity, chunk, mesh] : view.each()) {
        if(mesh.slot == std::numeric_limits<std::uint32_t>::max()) {
            continue; // invalid slot
        }

        s_chunk_positions[mesh.slot] = utils::to_fvec(chunk.position - camera::chunk);
    }

    glBindBuffer(GL_TEXTURE_BUFFER, s_chunk_positions_buffer);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, static_cast<GLsizeiptr>(max_slot * sizeof(Eigen::Vector3f)), s_chunk_positions.data());
}

static void batch_flush(void)
{
    if(s_batch_draw_index == 0) {
        return;
    }

    glMultiDrawElementsBaseVertex(GL_TRIANGLES, s_multidraw_counts.data(), GL_UNSIGNED_INT, s_multidraw_indices.data(),
        static_cast<GLsizei>(s_batch_draw_index), s_multidraw_basevertices.data());

    globals::num_draw_calls += 1;
    globals::num_draw_vertices += s_batch_vertex_count;

    s_batch_draw_index = 0;
    s_batch_vertex_count = 0;
}

static void batch_append(const ChunkMesh_Part& part)
{
    if(part.count == 0) {
        return;
    }

    auto quad_count = part.count / 4;

    if(quad_count == 0) {
        return;
    }

    if(s_batch_draw_index >= BATCH_MAX_PARTS) {
        batch_flush();
    }

    ensure_multidraw_capacity(s_batch_draw_index + 1);

    s_multidraw_counts[s_batch_draw_index] = static_cast<GLsizei>(quad_count * 6);
    s_multidraw_basevertices[s_batch_draw_index] = static_cast<GLint>(part.base_vertex);
    s_multidraw_indices[s_batch_draw_index] = reinterpret_cast<const void*>(pattern_offset(quad_count));

    s_batch_draw_index += 1;
    s_batch_vertex_count += part.count;
}

static float chunk_distance_sq(entt::entity entity)
{
    auto& chunk_registry = globals::world->chunk_registry();
    const auto& chunk = chunk_registry.get<Chunk_Component>(entity);
    Eigen::Vector3f position = utils::to_fvec(chunk.position - camera::chunk) * static_cast<float>(constant::CHUNK_SIZE);
    Eigen::Vector3f delta = position - camera::local;
    return delta.squaredNorm();
}

static void update_sorted_chunks(void)
{
    auto& chunk_registry = globals::world->chunk_registry();
    auto view = chunk_registry.view<Chunk_Component, ChunkMesh>();

    s_sorted_chunks.clear();
    s_sorted_chunks.reserve(view.size_hint());

    for(const auto [entity, chunk, mesh] : view.each()) {
        auto has_any = false;
        has_any = has_any || mesh.opaque.count;
        has_any = has_any || mesh.alpha.count;
        has_any = has_any || mesh.fluid.count;

        if(has_any) {
            s_sorted_chunks.push_back(std::make_pair(entity, chunk_distance_sq(entity)));
        }
    }

    std::sort(s_sorted_chunks.begin(), s_sorted_chunks.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });
}

static void on_chunk_create(const ChunkCreateEvent& event)
{
    s_sorted_dirty = true;
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    s_sorted_dirty = true;
}

static void on_chunk_update(const ChunkUpdateEvent& event)
{
    s_sorted_dirty = true;
}

static void on_chunk_mesh(entt::entity entity)
{
    s_sorted_dirty = true;
}

static void bind_pipeline_state(unsigned fog_model)
{
    s_program.set_variant_frag(FOG_MODEL, fog_model);
    vx::throw_if_not(s_program.update());

    auto& vproj = camera::instance.view_projection();
    auto animation_timer = static_cast<std::uint32_t>(globals::current_tick >> 1);
    auto sky_light_color = Eigen::Vector3f(SKY_LIGHT_COLOR * fog::daylight * fog::light_scale);

    glUseProgram(s_program.handle);
    glUniformMatrix4fv(s_program.uniforms[su_ViewProjection].location, 1, GL_FALSE, vproj.data());
    glUniform1ui(s_program.uniforms[su_AnimationTimer].location, animation_timer);
    glUniform1f(s_program.uniforms[su_ViewDistance].location, fog::distance);
    glUniform3fv(s_program.uniforms[su_FogColor].location, 1, fog::color.data());
    glUniform3fv(s_program.uniforms[su_SkyColor].location, 1, sky_light_color.data());
    glUniform3fv(s_program.uniforms[su_BlockColor].location, 1, BLOCK_LIGHT_COLOR.data());
    glUniform1f(s_program.uniforms[su_Ambient].location, AMBIENT_LIGHT);
    glUniform3fv(s_program.uniforms[su_CameraLocal].location, 1, camera::local.data());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, block_atlas::texture);
    glUniform1i(s_program.uniforms[su_AtlasTexture].location, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, block_atlas::tbo_frames);
    glUniform1i(s_program.uniforms[su_AtlasFrames].location, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, block_atlas::tbo_strips);
    glUniform1i(s_program.uniforms[su_AtlasStrips].location, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_BUFFER, s_chunk_positions_tbo);
    glUniform1i(s_program.uniforms[su_ChunkPositions].location, 3);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk_vbo::handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_pattern_ibo);
}

void chunk_renderer::init(void)
{
    constexpr static std::size_t INITIAL_CAPACITY = 256;

    auto program_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "chunk");
    auto program_ok = s_program.setup(program_id);
    vx::throw_if_not_fmt(program_ok, "{}: setup failed", program_id.full_string());

    su_ViewProjection = s_program.add_uniform("u_ViewProjection");
    su_AnimationTimer = s_program.add_uniform("u_AnimationTimer");
    su_ViewDistance = s_program.add_uniform("u_ViewDistance");
    su_FogColor = s_program.add_uniform("u_FogColor");
    su_SkyColor = s_program.add_uniform("u_SkyColor");
    su_BlockColor = s_program.add_uniform("u_BlockColor");
    su_Ambient = s_program.add_uniform("u_Ambient");
    su_CameraLocal = s_program.add_uniform("u_CameraLocal");
    su_AtlasTexture = s_program.add_uniform("u_AtlasTexture");
    su_AtlasStrips = s_program.add_uniform("u_AtlasStrips");
    su_AtlasFrames = s_program.add_uniform("u_AtlasFrames");
    su_ChunkPositions = s_program.add_uniform("u_ChunkPositions");

    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);

    glBindBuffer(GL_ARRAY_BUFFER, chunk_vbo::handle);

    auto stride = static_cast<GLsizei>(sizeof(ChunkMesh_Vertex));
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, stride, (const void*)(offsetof(ChunkMesh_Vertex, data_1)));
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, stride, (const void*)(offsetof(ChunkMesh_Vertex, data_2)));
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, stride, (const void*)(offsetof(ChunkMesh_Vertex, data_3)));
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, stride, (const void*)(offsetof(ChunkMesh_Vertex, data_4)));
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, stride, (const void*)(offsetof(ChunkMesh_Vertex, data_5)));

    for(GLuint i = 0; i < 5; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 0);
    }

    s_pattern_offsets.clear();
    s_pattern_ibo_capacity = INITIAL_CAPACITY;
    s_pattern_ibo_size = 0;

    glGenBuffers(1, &s_pattern_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_pattern_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(s_pattern_ibo_capacity), nullptr, GL_STATIC_DRAW);

    s_chunk_positions.assign(INITIAL_CAPACITY, Eigen::Vector3f::Zero());
    s_chunk_positions_capacity = INITIAL_CAPACITY;

    glGenBuffers(1, &s_chunk_positions_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, s_chunk_positions_buffer);
    glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(s_chunk_positions_capacity * sizeof(Eigen::Vector3f)), nullptr,
        GL_DYNAMIC_DRAW);

    glGenTextures(1, &s_chunk_positions_tbo);
    glBindTexture(GL_TEXTURE_BUFFER, s_chunk_positions_tbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, s_chunk_positions_buffer);

    s_sorted_dirty = true;
    s_sorted_chunks.clear();

    s_batch_draw_index = 0;
    s_batch_vertex_count = 0;

    s_multidraw_counts.clear();
    s_multidraw_basevertices.clear();
    s_multidraw_indices.clear();

    ensure_multidraw_capacity(BATCH_MAX_PARTS);

    s_last_camera_chunk = camera::chunk + ChunkPos::Ones();

    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<ChunkUpdateEvent>().connect<&on_chunk_update>();

    auto& chunk_registry = globals::world->chunk_registry();
    chunk_registry.on_update<ChunkMesh>().connect<&on_chunk_mesh>();
    chunk_registry.on_destroy<ChunkMesh>().connect<&on_chunk_mesh>();
}

void chunk_renderer::shutdown(void)
{
    auto& chunk_registry = globals::world->chunk_registry();
    chunk_registry.on_update<ChunkMesh>().disconnect<&on_chunk_mesh>();
    chunk_registry.on_destroy<ChunkMesh>().disconnect<&on_chunk_mesh>();

    glDeleteBuffers(1, &s_chunk_positions_buffer);
    glDeleteTextures(1, &s_chunk_positions_tbo);
    glDeleteBuffers(1, &s_pattern_ibo);
    glDeleteVertexArrays(1, &s_vao);

    s_chunk_positions.clear();
    s_pattern_offsets.clear();
    s_multidraw_counts.clear();
    s_multidraw_basevertices.clear();
    s_multidraw_indices.clear();

    s_program.destroy();
}

void chunk_renderer::prepare(void)
{
    ZoneScoped;

    s_frame_ready = false;

    if(block_atlas::texture == 0 || block_atlas::tbo_frames == 0 || block_atlas::tbo_strips == 0) {
        return;
    }

    auto camera_chunk_changed = !camera::chunk.isApprox(s_last_camera_chunk);
    auto needs_tbo_update = camera_chunk_changed || s_sorted_dirty;

    if(s_sorted_dirty || camera_chunk_changed) {
        s_last_camera_chunk = camera::chunk;
        s_sorted_dirty = false;
        update_sorted_chunks();
    }

    if(needs_tbo_update) {
        update_chunk_positions_tbo();
    }

    s_frame_ready = true;
}

void chunk_renderer::render_opaque(void)
{
    ZoneScoped;

    if(!s_frame_ready || s_sorted_chunks.empty()) {
        return;
    }

    bind_pipeline_state(fog::model.value());

    auto& frustum = camera::instance.frustum();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);

    auto& chunk_registry = globals::world->chunk_registry();

    for(const auto& it : s_sorted_chunks) {
        const auto& mesh = chunk_registry.get<ChunkMesh>(it.first);
        const auto& chunk = chunk_registry.get<Chunk_Component>(it.first);

        if(mesh.opaque.count && frustum.intersects(utils::bounds(chunk.position - camera::chunk))) {
            batch_append(mesh.opaque);
        }
    }

    batch_flush();
}

void chunk_renderer::render_alpha(void)
{
    ZoneScoped;

    if(!s_frame_ready || s_sorted_chunks.empty()) {
        return;
    }

    bind_pipeline_state(fog::model.value());

    auto& frustum = camera::instance.frustum();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    auto& chunk_registry = globals::world->chunk_registry();

    for(auto it = s_sorted_chunks.rbegin(); it != s_sorted_chunks.rend(); it = std::next(it)) {
        const auto& mesh = chunk_registry.get<ChunkMesh>(it->first);
        const auto& chunk = chunk_registry.get<Chunk_Component>(it->first);

        if(mesh.alpha.count && frustum.intersects(utils::bounds(chunk.position - camera::chunk))) {
            batch_append(mesh.alpha);
        }
    }

    batch_flush();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
}

void chunk_renderer::render_fluid(void)
{
    ZoneScoped;

    if(!s_frame_ready || s_sorted_chunks.empty()) {
        return;
    }

    bind_pipeline_state(fog::model.value());

    auto& frustum = camera::instance.frustum();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    auto& chunk_registry = globals::world->chunk_registry();

    for(auto it = s_sorted_chunks.rbegin(); it != s_sorted_chunks.rend(); it = std::next(it)) {
        const auto& mesh = chunk_registry.get<ChunkMesh>(it->first);
        const auto& chunk = chunk_registry.get<Chunk_Component>(it->first);

        if(mesh.fluid.count && frustum.intersects(utils::bounds(chunk.position - camera::chunk))) {
            batch_append(mesh.fluid);
        }
    }

    batch_flush();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
}
