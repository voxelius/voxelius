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
#include "shared/utils/coord.hh"
#include "shared/world/weather.hh"
#include "shared/world/world.hh"
#include "shared/world/world_definition.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/settings.hh"
#include "client/shader_program.hh"
#include "client/utils/world.hh"
#include "client/world/cloud_field.hh"
#include "client/world/cloud_mesher.hh"
#include "client/world/skybox.hh"

constexpr static float WIND_EASE_PER_SECOND = 6.0f;
constexpr static float BUILD_COOLDOWN_SECONDS = 0.2f; // A build closer than this to the previous one would not pay for itself
constexpr static float CLOUD_NEAR_PLANE = 1.0f;
constexpr static float CLOUD_DEPTH_MARGIN = 1024.0f;

// A deck is anchored to the field drift at the time it was built, so a
// rebuild resamples only the cells that slid in at the window's edge
struct CloudDrift final {
    Eigen::Vector2f anchor { Eigen::Vector2f::Zero() };
    Eigen::Vector2f offset { Eigen::Vector2f::Zero() };
    bool anchored { false };
};

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

static cvar::Boolean s_clouds { true };
static gui::CheckBox s_clouds_gui;

static ShaderProgram s_program;
static std::size_t su_CloudViewProjection;
static std::size_t su_CameraOrigin;
static std::size_t su_DeckCenter;
static std::size_t su_WindOffset;
static std::size_t su_SunDirection;
static std::size_t su_SunColor;
static std::size_t su_SkyColor;
static std::size_t su_HazeColor;
static std::size_t su_HazeRange;
static std::size_t su_CameraPosition;
static std::size_t su_FadeDistance;
static std::size_t su_BlendRange;
static std::size_t su_DepthBias;

static GLuint s_vao[2];
static GLuint s_vbo[2];
static GLuint s_ibo[2];

static CloudMesh s_meshes[2];
static std::size_t s_front;
static std::vector<CloudGroup> s_groups;
static CloudDrift s_drift;
static Eigen::Vector2f s_last_build_offset;
static std::uint64_t s_state_signature; // Weather seed the current mesh was built from
static std::uint64_t s_generation;
static Eigen::Vector3f s_last_build_center { Eigen::Vector3f::Zero() };
static unsigned s_last_cloud_range { 0 };
static bool s_building { false };
static bool s_rebuild_requested { false };
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

static bool needs_rebuild(void)
{
    // A reset empties the meshes while the latches below still describe
    // the build that went away, so the first frame after it has to rebuild
    if(s_rebuild_requested) {
        return true;
    }

    if(s_last_cloud_range != camera::cloud_range.value()) {
        return true;
    }

    // The deck is in world space, so leaving a chunk does not invalidate it; only a window that slid far enough does
    if((camera_world_position() - s_last_build_center).norm() > cloud_field::fine_cell_blocks() * 4.0f) {
        return true;
    }

    // The mesh is only re-evaluated when the field has drifted far
    // enough for its window to slide a cell; the wind offset alone
    // keeps the deck moving in between, and the deck is sampled in
    // the frame of the build, so a rebuild does not reshuffle cubes
    if((s_drift.offset - s_last_build_offset).norm() > cloud_field::fine_cell_blocks() * 2.0f) {
        return true;
    }

    // The deck is sampled in the anchor frame, so its pattern only changes
    // on a reseed; the drift is covered by the offset above
    return s_state_signature != globals::world->weather().seed();
}

// The deck is sampled in the weather field and drawn with that field's
// drift, so the cubes keep the shape the field gave them while the front
// travels. Driving the offset from the locally integrated wind instead lets
// the two slowly disagree, and the next rebuild then swaps a whole swath of
// the deck at once
static void advance_wind_offsets(void)
{
    auto step = 1.0f - std::exp(-WIND_EASE_PER_SECOND * std::min(globals::window_frametime, 0.1f));
    auto& state = globals::world->weather();
    auto field_drift = state.drift(globals::world->weather_tick);
    auto& drift = s_drift;

    if(!drift.anchored) {
        drift.anchor = field_drift - drift.offset;
        drift.anchored = true;
    }

    // A jump of the weather clock, such as a forced front, must not
    // teleport the deck across the sky; a short ease turns it into a slide.
    // Ordinary drift moves at a couple of blocks per second, so the lag
    // this adds is under a block
    auto target = field_drift - drift.anchor;

    drift.offset += (target - drift.offset) * step;
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

    auto& definition = globals::world->definition();

    CloudInput input {};
    input.state = globals::world->weather();
    input.allowed = definition.weather;
    input.anchor = s_drift.anchor;
    input.offset = s_drift.offset;
    input.center = camera_position;
    input.radius = radius;

    s_last_build_offset = s_drift.offset;

    s_groups.clear();
    cloud_field::collect_groups(camera_position.y(), s_groups);

    // Far decks first: clouds blend, so the order is what makes overlapping layers stack correctly
    std::sort(s_groups.begin(), s_groups.end(), [](const CloudGroup& a, const CloudGroup& b) {
        return a.distance > b.distance;
    });

    s_last_build_center = camera_position;
    s_last_cloud_range = camera::cloud_range.value();
    s_state_signature = globals::world->weather().seed();
    s_rebuild_requested = false;
    s_build_cooldown = BUILD_COOLDOWN_SECONDS;

    s_generation += 1;
    s_building = true;

    threading::submit<CloudTask>(std::move(input), s_groups, std::move(s_meshes[1 - s_front]), s_generation);
}

void cloud_renderer::init(void)
{
    update_depth_range(0.0f);

    auto& r_clouds_binding = cvar::bind_var(s_clouds, "r_clouds");
    r_clouds_binding.set_help("Draw the cloud deck: 0 = off, 1 = on");
    r_clouds_binding.set_flags(FCVAR_ARCHIVE);

    s_clouds_gui.bind("r_clouds");
    settings::video_atmosphere.add_child(s_clouds_gui, 1);

    auto cloud_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "cloud");
    auto cloud_ok = s_program.setup(cloud_id);
    vx::throw_if_not_fmt(cloud_ok, "{}: setup failed", cloud_id.full_string());

    su_CloudViewProjection = s_program.add_uniform("u_CloudViewProjection");
    su_CameraOrigin = s_program.add_uniform("u_CameraOrigin");
    su_DeckCenter = s_program.add_uniform("u_DeckCenter");
    su_WindOffset = s_program.add_uniform("u_WindOffset");
    su_SunDirection = s_program.add_uniform("u_SunDirection");
    su_SunColor = s_program.add_uniform("u_SunColor");
    su_SkyColor = s_program.add_uniform("u_SkyColor");
    su_HazeColor = s_program.add_uniform("u_HazeColor");
    su_HazeRange = s_program.add_uniform("u_HazeRange");
    su_CameraPosition = s_program.add_uniform("u_CameraPosition");
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
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(CloudVertex, normal)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(CloudVertex, color)));
        glEnableVertexAttribArray(2);

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

    auto& visuals = utils::world_visuals();

    if(!visuals.clouds) {
        return;
    }

    advance_wind_offsets();

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
    auto camera_position = camera_world_position();

    auto sun_direction = skybox::sun_direction;
    auto sun_color = Eigen::Vector3f(skybox::sun_tint * (skybox::sun_strength * 1.5f));
    auto sky_color = Eigen::Vector3f((skybox::zenith_color + skybox::horizon_color) * 0.5f);
    auto haze_color = Eigen::Vector3f(fog::color * 0.55f + Eigen::Vector3f(0.80f, 0.82f, 0.86f) * (0.45f * fog::daylight));

    auto haze_end = std::max(cloud_renderer::z_far - CLOUD_DEPTH_MARGIN, 1.0f);
    auto haze_start = haze_end * 0.35f;

    vx::throw_if_not(s_program.update());

    glUseProgram(s_program.handle);

    glUniformMatrix4fv(s_program.uniforms[su_CloudViewProjection].location, 1, GL_FALSE, cloud_projection.data());
    glUniform3fv(s_program.uniforms[su_CameraOrigin].location, 1, camera_origin.data());
    glUniform3fv(s_program.uniforms[su_SunDirection].location, 1, sun_direction.data());
    glUniform3fv(s_program.uniforms[su_SunColor].location, 1, sun_color.data());
    glUniform3fv(s_program.uniforms[su_SkyColor].location, 1, sky_color.data());
    glUniform3fv(s_program.uniforms[su_HazeColor].location, 1, haze_color.data());
    glUniform2f(s_program.uniforms[su_HazeRange].location, haze_start, haze_end);
    glUniform3fv(s_program.uniforms[su_CameraPosition].location, 1, camera_position.data());

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
        glUniform2fv(s_program.uniforms[su_DeckCenter].location, 1, batch.center.data());
        glUniform3f(s_program.uniforms[su_WindOffset].location, s_drift.offset.x(), 0.0f, s_drift.offset.y());
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
    s_drift = CloudDrift {};
    s_last_build_offset = Eigen::Vector2f::Zero();
    s_generation += 1;
    s_building = false;
    s_rebuild_requested = true;
    s_front = 0;

    s_meshes[0] = CloudMesh {};
    s_meshes[1] = CloudMesh {};

    upload_mesh(s_meshes[s_front], s_front);
}
