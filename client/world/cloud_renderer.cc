#include "client/pch.hh"

#include "client/world/cloud_renderer.hh"

#include "core/camera.hh"
#include "core/cvar/boolean.hh"
#include "core/cvar/host.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/threading.hh"
#include "core/utils/angles.hh"

#include "shared/constant.hh"
#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"
#include "shared/world/weather.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/settings.hh"
#include "client/shader_program.hh"
#include "client/world/cloud_field.hh"
#include "client/world/cloud_mesher.hh"
#include "client/world/skybox.hh"

constexpr static float WIND_EASE_PER_SECOND = 6.0f;
constexpr static float BUILD_COOLDOWN_SECONDS = 0.2f; // A build closer than this to the previous one would not pay for itself
constexpr static float CLOUD_NEAR_PLANE = 1.0f;
constexpr static float CLOUD_DEPTH_MARGIN = 1024.0f;

static cvar::Boolean s_clouds { true };
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

float cloud_renderer::z_near = CLOUD_NEAR_PLANE;
float cloud_renderer::z_far = CLOUD_NEAR_PLANE + CLOUD_DEPTH_MARGIN;

static Eigen::Vector3f camera_world_position(void)
{
    return utils::to_fvec(camera::chunk) + camera::local;
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
    if((camera_world_position() - s_last_build_center).norm() > cloud_field::fine_cell_blocks() * 4.0f) {
        return true;
    }

    // The mesh is only re-evaluated when the field has drifted far
    // enough for the window to slide a cell; the wind offset alone keeps
    // the deck moving in between, and the field is sampled in the
    // same frame the deck lives in, so a rebuild does not reshuffle cubes
    if((s_wind_offset - s_last_build_offset).norm() > cloud_field::fine_cell_blocks() * 2.0f) {
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

    if(status.load(std::memory_order_relaxed) == task_status::CANCELLED) {
        m_mesh = CloudMesh {};
        return;
    }

    cloud_mesher::build(m_input, m_groups, m_mesh);
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
    auto radius = std::min(static_cast<float>(constant::CHUNK_SIZE * camera::cloud_range.value()), cloud_field::max_radius_blocks());

    update_depth_range(radius);

    CloudInput input {};
    auto states = weather::states();
    std::copy(states.begin(), states.end(), input.states.begin());
    input.anchor = s_drift_anchor;
    input.center = camera_position - s_wind_offset;
    input.radius = radius;

    s_groups.clear();
    cloud_field::collect_groups(camera_position.y(), s_groups);

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
    settings::video_atmosphere.add_child(s_clouds_gui, 1);

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
