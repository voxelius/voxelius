#include "client/pch.hh"

#include "client/world/precip_renderer.hh"

#include "core/camera.hh"
#include "core/cvar/boolean.hh"
#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/msg/log.hh"
#include "core/res/resource.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/weather.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/chunk.hh"
#include "shared/world/fluid.hh"
#include "shared/world/precip.hh"
#include "shared/world/precip_registry.hh"
#include "shared/world/weather.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/globals.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/res/texture_2d.hh"
#include "client/settings.hh"
#include "client/shader_program.hh"
#include "client/world/columns.hh"

constexpr static unsigned MAX_RADIUS = 16;
constexpr static std::size_t MAX_COLUMNS = (2 * MAX_RADIUS + 1) * (2 * MAX_RADIUS + 1);
constexpr static std::size_t MAX_VERTICES = MAX_COLUMNS * 6;
constexpr static float V_TILE_SCALE = 0.25f; // One texture tile spans four blocks along V
constexpr static float V_PHASE_RANGE = 4.0f; // The random phase covers four tiles
constexpr static float TIME_WRAP = 3600.0f;  // The scroll offset restarts once an hour, in seconds
constexpr static std::uint32_t PHASE_SALT = UINT32_C(0x9E3779B9);

struct PrecipVertex final {
    float position[3]; // Relative to client_camera::chunk, like the chunk meshes
    float uv[2];
    float color[4]; // Tint in RGB, column amount in A
};

static_assert(sizeof(PrecipVertex) == 9 * sizeof(float));
static_assert(0x00 == offsetof(PrecipVertex, position));
static_assert(0x0C == offsetof(PrecipVertex, uv));
static_assert(0x14 == offsetof(PrecipVertex, color));

struct PrecipColumn final {
    float x; // Column center, in blocks of the camera's chunk
    float z;
    float bottom; // Same space, in blocks above the chunk origin
    float top;
    float v0; // World anchored texture coordinates, so a chunk crossing does not shift them
    float v1;
    float dist2;
    const PrecipDefinition* definition;
    float effective_intensity;
};

struct DrawRange final {
    const PrecipDefinition* definition;
    GLint first;
    GLsizei count;
};

static ShaderProgram s_program;
static std::size_t su_ViewProjection;
static std::size_t su_CameraLocal;
static std::size_t su_Scroll;
static std::size_t su_Texture;
static std::size_t su_FogColor;
static std::size_t su_ViewDistance;

static GLuint s_vao;
static GLuint s_vbo;
static std::vector<PrecipVertex> s_vertices;
static std::vector<PrecipColumn> s_columns;
static std::vector<DrawRange> s_ranges;
static vx::hash_map<precip_type_id, res::handle<Texture2D>> s_textures;

static cvar::Boolean s_precip { true };
static cvar::Number<unsigned> s_radius { 8, 3, MAX_RADIUS };
static gui::CheckBox s_precip_gui;
static gui::SliderUnsigned s_radius_gui;

static float s_time;

// A surface block stops the column; a bush or a tuft
// does not, and a body of fluid is a surface of its own
static bool is_surface(const World* world, const BlockPos& pos)
{
    assert(world);

    if(auto id = world->get_block(pos)) {
        auto definition = block_registry::find_definition(id);

        if(definition && !definition->replaceable) {
            return true;
        }
    }

    if(world->get_fluid(pos)) {
        return true;
    }

    return false;
}

static std::optional<int> surface_between(const World* world, int x, int z, int top, int bottom)
{
    for(auto y = top; y >= bottom; --y) {
        if(is_surface(world, BlockPos(x, y, z))) {
            return y;
        }
    }

    return std::nullopt;
}

static float column_phase(int x, int z, std::uint32_t seed)
{
    return utils::weather_cell(x, z, seed ^ PHASE_SALT) * V_PHASE_RANGE;
}

// The camera sits in the local space of its own chunk, and the render works in
// that space; a world position is built only for a lookup and for the cell hash
static void build_columns(const World* world, const PrecipContext& ctx, const Eigen::Vector3f& camera)
{
    assert(world);

    s_columns.clear();

    auto radius = static_cast<int>(s_radius.value());

    auto origin_x = client_camera::chunk.x() << constant::CHUNK_SIZE_LOG2;
    auto origin_y = client_camera::chunk.y() << constant::CHUNK_SIZE_LOG2;
    auto origin_z = client_camera::chunk.z() << constant::CHUNK_SIZE_LOG2;

    auto base_x = static_cast<int>(std::floor(camera.x()));
    auto base_y = static_cast<int>(std::floor(camera.y()));
    auto base_z = static_cast<int>(std::floor(camera.z()));

    auto world_eye_y = static_cast<float>(origin_y + base_y);

    auto& weather_state = world->weather();
    auto seed = static_cast<std::uint32_t>(weather_state.seed());

    // The scan window only serves a column whose slice has not arrived yet
    auto scan_top = origin_y + base_y + radius;
    auto scan_bottom = origin_y + base_y - 2 * radius;

    for(auto dz = -radius; dz <= radius; dz += 1) {
        for(auto dx = -radius; dx <= radius; dx += 1) {
            auto world_x = origin_x + base_x + dx;
            auto world_z = origin_z + base_z + dz;

            auto world_surface = static_cast<std::int64_t>(origin_y + base_y);
            auto column_data = columns::find(BlockPosXZ(world_x, world_z));

            if(column_data && column_data->surface_y != Column::UNSET) {
                auto surface_y = column_data->surface_y;

                if(column_data->liquid_y != Column::UNSET && column_data->liquid_y > surface_y) {
                    surface_y = column_data->liquid_y;
                }

                world_surface = surface_y + 1;
            }
            else if(auto surface = surface_between(world, world_x, world_z, scan_top, scan_bottom)) {
                world_surface = surface.value() + 1;
            }

            // An XZ column can be loaded while the chunk at the camera height is not, so the column decides
            auto bpos = BlockPos(world_x, world_surface, world_z);

            if(world->find_chunk(utils::to_chunk(bpos)) == nullptr) {
                continue;
            }

            auto temperature = world->get_temperature(bpos, ctx.coverage);
            auto humidity = world->get_humidity_base(bpos);

            auto selection = precip::select(ctx, temperature, humidity);

            if(selection.type == PRECIP_ID_NULL) {
                continue;
            }

            auto definition = precip_registry::find_definition(selection.type);

            if(definition == nullptr || !definition->texture.is_valid()) {
                continue;
            }

            auto probability = selection.effective_intensity * definition->draw_chance;

            if(probability <= 0.0f) {
                continue;
            }

            // The cell hash keeps the thinned set stable between frames
            if(utils::weather_cell(world_x, world_z, seed) >= probability) {
                continue;
            }

            auto ground_y = static_cast<float>(world_surface);

            // The band is a box around the eye cut off by the ground; a lake would otherwise stack quads up to the eye
            auto world_top = world_eye_y + definition->band_half_height;
            auto world_bottom = std::max(ground_y, world_eye_y - definition->band_half_height);

            auto local_x = static_cast<float>(base_x + dx) + 0.5f;
            auto local_z = static_cast<float>(base_z + dz) + 0.5f;
            auto local_bottom = world_bottom - static_cast<float>(origin_y);
            auto local_top = world_top - static_cast<float>(origin_y);

            if(local_top <= local_bottom) {
                continue;
            }

            auto delta_x = local_x - camera.x();
            auto delta_z = local_z - camera.z();
            auto phase = column_phase(world_x, world_z, seed);

            PrecipColumn column {};
            column.x = local_x;
            column.z = local_z;
            column.bottom = local_bottom;
            column.top = local_top;
            column.v0 = world_bottom * V_TILE_SCALE + phase;
            column.v1 = world_top * V_TILE_SCALE + phase;
            column.dist2 = delta_x * delta_x + delta_z * delta_z;
            column.definition = definition;
            column.effective_intensity = selection.effective_intensity;
            s_columns.push_back(column);
        }
    }
}

static void build_vertices(const Eigen::Vector3f& camera)
{
    s_vertices.clear();
    s_ranges.clear();

    // Batched by type, and the farthest column of a run goes first because the quads blend in order
    std::stable_sort(s_columns.begin(), s_columns.end(), [](const PrecipColumn& a, const PrecipColumn& b) {
        if(a.definition->id == b.definition->id) {
            return a.dist2 > b.dist2;
        }
        else {
            return a.definition->id < b.definition->id;
        }
    });

    auto light = fog::daylight * fog::light_scale;

    for(const auto& column : s_columns) {
        // The column center and the camera are
        // both in the blocks of the camera's chunk
        auto dx = column.x - camera.x();
        auto dz = column.z - camera.z();
        auto length = std::sqrt(dx * dx + dz * dz);

        if(length < 0.001f) {
            continue;
        }

        // The column turns its face to the camera around
        // Y, so it never reads as a wall at a grazing angle
        auto tangent_x = -dz / length;
        auto tangent_z = dx / length;
        auto half = column.definition->quad_width * 0.5f;

        auto left_x = column.x - tangent_x * half;
        auto left_z = column.z - tangent_z * half;
        auto right_x = column.x + tangent_x * half;
        auto right_z = column.z + tangent_z * half;

        auto red = column.definition->color.x() * light;
        auto green = column.definition->color.y() * light;
        auto blue = column.definition->color.z() * light;
        auto alpha = std::clamp(column.effective_intensity * column.definition->opacity, 0.0f, 1.0f);

        auto first = static_cast<GLint>(s_vertices.size());

        auto emit = [&](float x, float y, float z, float u, float v) {
            PrecipVertex vertex {};
            vertex.position[0] = x;
            vertex.position[1] = y;
            vertex.position[2] = z;
            vertex.uv[0] = u;
            vertex.uv[1] = v;
            vertex.color[0] = red;
            vertex.color[1] = green;
            vertex.color[2] = blue;
            vertex.color[3] = alpha;
            s_vertices.push_back(vertex);
        };

        emit(left_x, column.bottom, left_z, 0.0f, column.v0);
        emit(right_x, column.bottom, right_z, 1.0f, column.v0);
        emit(right_x, column.top, right_z, 1.0f, column.v1);

        emit(left_x, column.bottom, left_z, 0.0f, column.v0);
        emit(right_x, column.top, right_z, 1.0f, column.v1);
        emit(left_x, column.top, left_z, 0.0f, column.v1);

        auto count = static_cast<GLsizei>(s_vertices.size()) - first;

        if(s_ranges.empty() || s_ranges.back().definition != column.definition) {
            DrawRange range {};
            range.definition = column.definition;
            range.first = first;
            range.count = count;
            s_ranges.push_back(range);
        }
        else {
            s_ranges.back().count += count;
        }
    }
}

void precip_renderer::init(void)
{
    auto& r_precip_binding = cvar::bind_var(s_precip, "r_precip");
    r_precip_binding.set_help("Draw falling precipitation: 0 = off, 1 = on");
    r_precip_binding.set_flags(FCVAR_ARCHIVE);

    s_precip_gui.bind("r_precip");
    settings::video_atmosphere.add_child(s_precip_gui, 1);

    auto& r_precip_radius_binding = cvar::bind_var(s_radius, "r_precip_radius");
    r_precip_radius_binding.set_help("Radius of the precipitation volume, in blocks");
    r_precip_radius_binding.set_flags(FCVAR_ARCHIVE);

    s_radius_gui.set_range(3, MAX_RADIUS);
    s_radius_gui.bind("r_precip_radius");
    settings::video_atmosphere.add_child(s_radius_gui, 2);

    auto program_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "precip");
    auto program_ok = s_program.setup(program_id);
    vx::throw_if_not_fmt(program_ok, "{}: setup failed", program_id.full_string());

    su_ViewProjection = s_program.add_uniform("u_ViewProjection");
    su_CameraLocal = s_program.add_uniform("u_CameraLocal");
    su_Scroll = s_program.add_uniform("u_Scroll");
    su_Texture = s_program.add_uniform("u_Texture");
    su_FogColor = s_program.add_uniform("u_FogColor");
    su_ViewDistance = s_program.add_uniform("u_ViewDistance");

    for(const auto& definition : precip_registry::all_definitions()) {
        if(!definition.texture.is_valid()) {
            continue;
        }

        auto texture = res::load<Texture2D>(definition.texture, "textures/precip", ".png", RESFLAG_TEX_NEAREST);

        if(texture == nullptr) {
            msg::log_warning("{}: texture load failed", definition.texture.full_string());
            continue;
        }

        s_textures.try_emplace(definition.id, std::move(texture));
    }

    constexpr auto STRIDE = static_cast<GLsizei>(sizeof(PrecipVertex));

    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);

    glGenBuffers(1, &s_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, reinterpret_cast<const void*>(offsetof(PrecipVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, reinterpret_cast<const void*>(offsetof(PrecipVertex, uv)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, STRIDE, reinterpret_cast<const void*>(offsetof(PrecipVertex, color)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Compile at boot: a broken shader must fail loudly
    // before the first storm instead of on the first drop
    vx::throw_if_not(s_program.update());

    s_columns.reserve(MAX_COLUMNS);
    s_vertices.reserve(MAX_VERTICES);
    s_ranges.reserve(MAX_COLUMNS);
}

void precip_renderer::shutdown(void)
{
    s_textures.clear();
    s_vertices.clear();
    s_columns.clear();
    s_ranges.clear();

    glDeleteBuffers(1, &s_vbo);
    glDeleteVertexArrays(1, &s_vao);

    s_vbo = 0;
    s_vao = 0;

    s_program.destroy();
}

void precip_renderer::render(void)
{
    ZoneScoped;

    s_time += globals::window_frametime;

    // A float texture coordinate loses precision after hours of play, so the offset wraps
    if(s_time > TIME_WRAP) {
        s_time = std::fmod(s_time, TIME_WRAP);
    }

    if(!s_precip.value() || globals::world == nullptr) {
        return;
    }

    auto world = globals::world;
    auto& definition = world->definition();

    if(definition.weather.empty()) {
        return;
    }

    if(client_camera::inside_fluid) {
        return;
    }

    // The weather field is sampled by world position; the geometry is built in the camera's chunk
    auto world_position = utils::to_fvec(client_camera::chunk) + client_camera::local;
    auto ctx = precip::context(world, definition.weather, world_position.x(), world_position.z());

    if(ctx.definition == nullptr || ctx.effective_intensity <= 0.0f) {
        return;
    }

    build_columns(world, ctx, client_camera::local);

    if(s_columns.empty()) {
        return;
    }

    build_vertices(client_camera::local);

    if(s_vertices.empty()) {
        return;
    }

    vx::throw_if_not(s_program.update());

    glUseProgram(s_program.handle);

    auto& view_projection = client_camera::instance.view_projection();
    glUniformMatrix4fv(s_program.uniforms[su_ViewProjection].location, 1, GL_FALSE, view_projection.data());
    glUniform3fv(s_program.uniforms[su_CameraLocal].location, 1, client_camera::local.data());
    glUniform3fv(s_program.uniforms[su_FogColor].location, 1, fog::color.data());
    glUniform1f(s_program.uniforms[su_ViewDistance].location, fog::distance);
    glUniform1i(s_program.uniforms[su_Texture].location, 0);

    // The next passes own their own state, so the culling state goes back as found
    auto cull_face = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    auto vertex_bytes = static_cast<GLsizeiptr>(s_vertices.size() * sizeof(PrecipVertex));
    glBufferData(GL_ARRAY_BUFFER, vertex_bytes, s_vertices.data(), GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);

    auto drawn_vertices = std::size_t(0);

    for(const auto& range : s_ranges) {
        auto it = s_textures.find(range.definition->id);

        if(it == s_textures.cend() || it->second == nullptr) {
            continue;
        }

        auto& range_definition = *range.definition;

        glBindTexture(GL_TEXTURE_2D, it->second->handle);
        glUniform2f(s_program.uniforms[su_Scroll].location, 0.0f, s_time * range_definition.fall_speed);
        glDrawArrays(GL_TRIANGLES, range.first, range.count);

        globals::num_draw_calls += 1;
        drawn_vertices += static_cast<std::size_t>(range.count);
    }

    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if(cull_face) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    globals::num_draw_vertices += drawn_vertices;
}
