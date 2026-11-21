#include "client/pch.hh"

#include "client/world/sky_renderer.hh"

#include "core/camera.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/res/resource.hh"
#include "core/utils/angles.hh"

#include "shared/constant.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/res/texture_2d.hh"
#include "client/shader_program.hh"
#include "client/utils/world.hh"
#include "client/world/skybox.hh"

// The sky spheres have to stay inside the camera's far plane
// even at the shortest view distance, which is twenty blocks
constexpr static float SKY_RADIUS = 10.0f;
constexpr static float STAR_MIN_SIZE = 0.020f;
constexpr static float STAR_MAX_SIZE = 0.040f;

constexpr static float STAR_MIN_BRIGHTNESS = 0.55f;
constexpr static float STAR_MAX_BRIGHTNESS = 1.0f;

// Half extents of the textured quads; the sun texture puts its disc
// at about half of this, so the visible sun is roughly six degrees
constexpr static float SUN_HALF_SIZE = 1.50f;
constexpr static float MOON_HALF_SIZE = 0.60f;

// The quads lie in the tangent plane of the sphere, so a
// star or a celestial body turns its face to the centre of the
// sky rather than shrinking to a line when the camera looks sideways
struct StarVertex final {
    Eigen::Vector3f position;
    float brightness;
    Eigen::Vector2f local;
};

static_assert(sizeof(StarVertex) == 6 * sizeof(float), "StarVertex must stay packed for the vertex layout");

static const Eigen::Vector4f SUN_UV_RECT { 0.0f, 0.0f, 1.0f, 1.0f };

static ShaderProgram s_sky;
static std::size_t su_CameraForward;
static std::size_t su_CameraRight;
static std::size_t su_CameraUp;
static std::size_t su_TanHalfFov;
static std::size_t su_Aspect;
static std::size_t su_HorizonColor;
static std::size_t su_SunHorizonColor;
static std::size_t su_ZenithColor;
static std::size_t su_GroundColor;
static std::size_t su_SunDirection;
static std::size_t su_SunTint;
static std::size_t su_SunStrength;
static std::size_t su_Twilight;

static ShaderProgram s_stars;
static std::size_t su_SkyViewProjection;
static std::size_t su_StarAngle;
static std::size_t su_StarTilt;
static std::size_t su_StarVisibility;

static ShaderProgram s_celestial;
static std::size_t su_CelestialViewProjection;
static std::size_t su_CelestialDirection;
static std::size_t su_CelestialRadius;
static std::size_t su_CelestialHalfSize;
static std::size_t su_CelestialRoll;
static std::size_t su_CelestialUvRect;
static std::size_t su_CelestialTexture;
static std::size_t su_CelestialTint;
static std::size_t su_CelestialOpacity;

static res::handle<Texture2D> s_sun_texture;
static res::handle<Texture2D> s_moon_texture;

static GLuint s_quad_vao;
static GLuint s_star_vao;
static GLuint s_star_vbo;
static GLsizei s_star_vertices;
static std::optional<std::pair<std::uint32_t, std::uint32_t>> s_built_stars;

static float next_float(std::mt19937_64& random)
{
    auto value = random();

    return static_cast<float>(value >> 11) / static_cast<float>(1ULL << 53);
}

// The moon atlas is four columns by two rows, with the phases
// laid out left to right and top to bottom in the image
static Eigen::Vector4f moon_uv_rect(std::uint32_t phase)
{
    constexpr std::uint32_t PHASES_PER_ROW = 4;

    // The atlas is 128x64; a half-texel inset keeps nearest
    // sampling from picking a neighbour cell at the edges
    constexpr static float TEXEL_WIDTH = 1.0f / 128.0f;
    constexpr static float TEXEL_HEIGHT = 1.0f / 64.0f;

    auto column = phase % PHASES_PER_ROW;
    auto row = phase / PHASES_PER_ROW;

    Eigen::Vector4f rect;
    rect.x() = static_cast<float>(column) * 0.25f + 0.5f * TEXEL_WIDTH;
    rect.y() = static_cast<float>(row) * 0.5f + 0.5f * TEXEL_HEIGHT;
    rect.z() = 0.25f - TEXEL_WIDTH;
    rect.w() = 0.5f - TEXEL_HEIGHT;

    return rect;
}

static void push_vertex(std::vector<StarVertex>& vertices, const Eigen::Vector3f& position, const Eigen::Vector2f& local, float brightness)
{
    StarVertex vertex {};
    vertex.position = position;
    vertex.brightness = brightness;
    vertex.local = local;

    vertices.push_back(vertex);
}

// The field is rebuilt only when the server seed changes; the
// star angle moves the whole field, so a still sky costs no upload
static void build_stars(void)
{
    auto& visuals = utils::world_visuals();
    auto count = static_cast<std::size_t>(visuals.stars ? visuals.stars->count : 0U);

    std::mt19937_64 random(skybox::star_seed);
    std::vector<StarVertex> vertices;

    vertices.reserve(count * 6);

    for(std::size_t i = 0; i < count; ++i) {
        Eigen::Vector3f direction;

        for(;;) {
            direction.x() = next_float(random) * 2.0f - 1.0f;
            direction.y() = next_float(random) * 2.0f - 1.0f;
            direction.z() = next_float(random) * 2.0f - 1.0f;

            auto length_squared = direction.squaredNorm();

            if(length_squared > 0.01f && length_squared < 1.0f) {
                break;
            }
        }

        direction.normalize();

        auto size = STAR_MIN_SIZE + (STAR_MAX_SIZE - STAR_MIN_SIZE) * next_float(random);
        auto brightness = STAR_MIN_BRIGHTNESS + (STAR_MAX_BRIGHTNESS - STAR_MIN_BRIGHTNESS) * next_float(random);

        Eigen::Vector3f helper = Eigen::Vector3f::UnitX();

        if(std::abs(direction.y()) < 0.99f) {
            helper = Eigen::Vector3f::UnitY();
        }

        auto tangent = direction.cross(helper).normalized();
        auto bitangent = direction.cross(tangent);

        auto roll = next_float(random) * std::numbers::pi_v<float> * 2.0f;
        auto u = tangent * std::cos(roll) + bitangent * std::sin(roll);
        auto v = bitangent * std::cos(roll) - tangent * std::sin(roll);

        auto center = direction * SKY_RADIUS;

        auto a = center - u * size - v * size;
        auto b = center + u * size - v * size;
        auto c = center + u * size + v * size;
        auto d = center - u * size + v * size;

        push_vertex(vertices, a, Eigen::Vector2f(-1.0f, -1.0f), brightness);
        push_vertex(vertices, b, Eigen::Vector2f(1.0f, -1.0f), brightness);
        push_vertex(vertices, c, Eigen::Vector2f(1.0f, 1.0f), brightness);

        push_vertex(vertices, a, Eigen::Vector2f(-1.0f, -1.0f), brightness);
        push_vertex(vertices, c, Eigen::Vector2f(1.0f, 1.0f), brightness);
        push_vertex(vertices, d, Eigen::Vector2f(-1.0f, 1.0f), brightness);
    }

    glBindBuffer(GL_ARRAY_BUFFER, s_star_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(StarVertex)), vertices.data(), GL_STATIC_DRAW);

    s_star_vertices = static_cast<GLsizei>(vertices.size());
    s_built_stars = std::make_pair(skybox::star_seed, static_cast<std::uint32_t>(count));
}

static void draw_stars(const Eigen::Matrix4f& sky_view_projection)
{
    if(s_star_vertices == 0) {
        return;
    }

    vx::throw_if_not(s_stars.update());

    glUseProgram(s_stars.handle);
    glUniformMatrix4fv(s_stars.uniforms[su_SkyViewProjection].location, 1, GL_FALSE, sky_view_projection.data());
    glUniform1f(s_stars.uniforms[su_StarAngle].location, skybox::phase * std::numbers::pi_v<float> * 2.0f);
    glUniform1f(s_stars.uniforms[su_StarTilt].location, skybox::sun_tilt);
    glUniform1f(s_stars.uniforms[su_StarVisibility].location, skybox::star_visibility);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(s_star_vao);
    glDrawArrays(GL_TRIANGLES, 0, s_star_vertices);
}

static void draw_celestial(const res::handle<Texture2D>& texture, const Eigen::Vector3f& direction, const Eigen::Vector3f& tint,
    const Eigen::Vector4f& uv_rect, float half_size, float opacity, bool additive, const Eigen::Matrix4f& sky_view_projection)
{
    if(texture == nullptr || opacity <= 0.001f) {
        return;
    }

    vx::throw_if_not(s_celestial.update());

    glUseProgram(s_celestial.handle);
    glUniformMatrix4fv(s_celestial.uniforms[su_CelestialViewProjection].location, 1, GL_FALSE, sky_view_projection.data());
    glUniform3fv(s_celestial.uniforms[su_CelestialDirection].location, 1, direction.data());
    glUniform1f(s_celestial.uniforms[su_CelestialRadius].location, SKY_RADIUS);
    glUniform1f(s_celestial.uniforms[su_CelestialHalfSize].location, half_size);
    glUniform1f(s_celestial.uniforms[su_CelestialRoll].location, 0.0f);
    glUniform4fv(s_celestial.uniforms[su_CelestialUvRect].location, 1, uv_rect.data());
    glUniform3fv(s_celestial.uniforms[su_CelestialTint].location, 1, tint.data());
    glUniform1f(s_celestial.uniforms[su_CelestialOpacity].location, opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glUniform1i(s_celestial.uniforms[su_CelestialTexture].location, 0);

    glEnable(GL_BLEND);

    if(additive) {
        glBlendFunc(GL_ONE, GL_ONE);
    }
    else {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    glBindVertexArray(s_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void sky_renderer::init(void)
{
    auto sky_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "sky");
    auto sky_ok = s_sky.setup(sky_id);
    vx::throw_if_not_fmt(sky_ok, "{}: setup failed", sky_id.full_string());

    su_CameraForward = s_sky.add_uniform("u_CameraForward");
    su_CameraRight = s_sky.add_uniform("u_CameraRight");
    su_CameraUp = s_sky.add_uniform("u_CameraUp");
    su_TanHalfFov = s_sky.add_uniform("u_TanHalfFov");
    su_Aspect = s_sky.add_uniform("u_Aspect");
    su_HorizonColor = s_sky.add_uniform("u_HorizonColor");
    su_SunHorizonColor = s_sky.add_uniform("u_SunHorizonColor");
    su_ZenithColor = s_sky.add_uniform("u_ZenithColor");
    su_GroundColor = s_sky.add_uniform("u_GroundColor");
    su_SunDirection = s_sky.add_uniform("u_SunDirection");
    su_SunTint = s_sky.add_uniform("u_SunTint");
    su_SunStrength = s_sky.add_uniform("u_SunStrength");
    su_Twilight = s_sky.add_uniform("u_Twilight");

    auto stars_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "stars");
    auto stars_ok = s_stars.setup(stars_id);
    vx::throw_if_not_fmt(stars_ok, "{}: setup failed", stars_id.full_string());

    su_SkyViewProjection = s_stars.add_uniform("u_SkyViewProjection");
    su_StarAngle = s_stars.add_uniform("u_StarAngle");
    su_StarTilt = s_stars.add_uniform("u_StarTilt");
    su_StarVisibility = s_stars.add_uniform("u_StarVisibility");

    auto celestial_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "celestial");
    auto celestial_ok = s_celestial.setup(celestial_id);
    vx::throw_if_not_fmt(celestial_ok, "{}: setup failed", celestial_id.full_string());

    su_CelestialViewProjection = s_celestial.add_uniform("u_SkyViewProjection");
    su_CelestialDirection = s_celestial.add_uniform("u_Direction");
    su_CelestialRadius = s_celestial.add_uniform("u_Radius");
    su_CelestialHalfSize = s_celestial.add_uniform("u_HalfSize");
    su_CelestialRoll = s_celestial.add_uniform("u_Roll");
    su_CelestialUvRect = s_celestial.add_uniform("u_UvRect");
    su_CelestialTexture = s_celestial.add_uniform("u_Texture");
    su_CelestialTint = s_celestial.add_uniform("u_Tint");
    su_CelestialOpacity = s_celestial.add_uniform("u_Opacity");

    auto sun_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "sun");
    s_sun_texture = res::load<Texture2D>(sun_id, "textures/environment", ".png", RESFLAG_TEX_NEAREST);

    if(!s_sun_texture) {
        msg::log_warning("{}: texture load failed", sun_id.full_string());
    }

    auto moon_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "moon_phases");
    s_moon_texture = res::load<Texture2D>(moon_id, "textures/environment", ".png", RESFLAG_TEX_NEAREST);

    if(!s_moon_texture) {
        msg::log_warning("{}: texture load failed", moon_id.full_string());
    }

    glGenVertexArrays(1, &s_quad_vao);

    glGenVertexArrays(1, &s_star_vao);
    glBindVertexArray(s_star_vao);

    glGenBuffers(1, &s_star_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_star_vbo);

    auto stride = static_cast<GLsizei>(sizeof(StarVertex));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(StarVertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(StarVertex, brightness)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(StarVertex, local)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    build_stars();
}

void sky_renderer::shutdown(void)
{
    glDeleteBuffers(1, &s_star_vbo);
    glDeleteVertexArrays(1, &s_star_vao);
    glDeleteVertexArrays(1, &s_quad_vao);

    s_star_vertices = 0;

    release_textures();

    s_celestial.destroy();
    s_stars.destroy();
    s_sky.destroy();
}

void sky_renderer::render(void)
{
    ZoneScoped;

    // The camera has no valid matrices until a player exists; the
    // flat clear colour is the right sky for the menu that follows
    auto view = camera::instance.view();
    auto projection = camera::instance.projection();

    if(view.hasNaN() || projection.hasNaN()) {
        return;
    }

    auto& visuals = utils::world_visuals();
    auto expected_count = visuals.stars ? visuals.stars->count : 0U;

    if(!s_built_stars.has_value()) {
        build_stars();
    }
    else if(s_built_stars->first != skybox::star_seed || s_built_stars->second != expected_count) {
        build_stars();
    }

    vx::throw_if_not(s_sky.update());

    auto half_fov = 0.5f * utils::radians(camera::vertical_fov.value());
    auto tan_half_fov = std::tan(half_fov);
    auto ground_color = Eigen::Vector3f(skybox::horizon_color * 0.35f);

    auto forward = camera::instance.forward_vector();
    auto right = camera::instance.right_vector();
    auto up = camera::instance.up_vector();

    // The sky spheres turn with the camera, so the view
    // matrix loses its translation and keeps only its rotation
    auto view_rotation = camera::instance.view();
    view_rotation(0, 3) = 0.0f;
    view_rotation(1, 3) = 0.0f;
    view_rotation(2, 3) = 0.0f;

    Eigen::Matrix4f sky_view_projection = camera::instance.projection() * view_rotation;

    glUseProgram(s_sky.handle);
    glUniform3fv(s_sky.uniforms[su_CameraForward].location, 1, forward.data());
    glUniform3fv(s_sky.uniforms[su_CameraRight].location, 1, right.data());
    glUniform3fv(s_sky.uniforms[su_CameraUp].location, 1, up.data());
    glUniform1f(s_sky.uniforms[su_TanHalfFov].location, tan_half_fov);
    glUniform1f(s_sky.uniforms[su_Aspect].location, globals::aspect);
    glUniform3fv(s_sky.uniforms[su_HorizonColor].location, 1, skybox::horizon_color.data());
    glUniform3fv(s_sky.uniforms[su_SunHorizonColor].location, 1, skybox::sun_horizon_color.data());
    glUniform3fv(s_sky.uniforms[su_ZenithColor].location, 1, skybox::zenith_color.data());
    glUniform3fv(s_sky.uniforms[su_GroundColor].location, 1, ground_color.data());
    glUniform3fv(s_sky.uniforms[su_SunDirection].location, 1, skybox::sun_direction.data());
    glUniform3fv(s_sky.uniforms[su_SunTint].location, 1, skybox::sun_tint.data());
    glUniform1f(s_sky.uniforms[su_SunStrength].location, skybox::sun_strength);
    glUniform1f(s_sky.uniforms[su_Twilight].location, skybox::twilight);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glBindVertexArray(s_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    auto draw_star_field = skybox::star_visibility > 0.001f;

    if(draw_star_field) {
        draw_stars(sky_view_projection);
    }

    auto draw_moon = skybox::moon_strength > 0.001f;

    if(draw_moon) {
        auto uv_rect = moon_uv_rect(skybox::moon_phase);

        draw_celestial(s_moon_texture, skybox::moon_direction, skybox::moon_tint, uv_rect, MOON_HALF_SIZE, skybox::moon_strength, false,
            sky_view_projection);
    }

    auto draw_sun_body = skybox::sun_strength > 0.001f;

    if(draw_sun_body) {
        draw_celestial(s_sun_texture, skybox::sun_direction, skybox::sun_tint, SUN_UV_RECT, SUN_HALF_SIZE, skybox::sun_strength, true,
            sky_view_projection);
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    globals::num_draw_calls += 1;
    globals::num_draw_vertices += 3;

    if(draw_star_field) {
        globals::num_draw_calls += 1;
        globals::num_draw_vertices += static_cast<std::size_t>(s_star_vertices);
    }

    if(draw_moon) {
        globals::num_draw_calls += 1;
        globals::num_draw_vertices += 4;
    }

    if(draw_sun_body) {
        globals::num_draw_calls += 1;
        globals::num_draw_vertices += 4;
    }
}

void sky_renderer::release_textures(void)
{
    s_sun_texture.reset();
    s_moon_texture.reset();
}
