#include "client/pch.hh"

#include "client/world/outline.hh"

#include "core/camera.hh"
#include "core/config/map.hh"
#include "core/config/ref.hh"
#include "core/exception.hh"
#include "core/identifier.hh"

#include "shared/utils/coord.hh"

#include "client/camera.hh"
#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/slider.hh"
#include "client/shader_program.hh"

static ShaderProgram s_program;
static std::size_t su_ViewProjection;
static std::size_t su_WorldPosition;
static std::size_t su_ViewDistance;
static std::size_t su_Modulate;
static std::size_t su_Scale;

static GLuint s_vao;
static GLuint s_vbo_cube;
static GLuint s_vbo_line;

void outline::init(void)
{
    auto program_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "outline");
    auto program_ok = s_program.setup(program_id);
    vx::throw_if_not_fmt(program_ok, "{}: setup failed", program_id.full_string());

    su_ViewProjection = s_program.add_uniform("u_ViewProjection");
    su_WorldPosition = s_program.add_uniform("u_WorldPosition");
    su_ViewDistance = s_program.add_uniform("u_ViewDistance");
    su_Modulate = s_program.add_uniform("u_Modulate");
    su_Scale = s_program.add_uniform("u_Scale");

    const Eigen::Vector3f cube_vertices[24] = {
        Eigen::Vector3f(0.0f, 0.0f, 0.0f),
        Eigen::Vector3f(0.0f, 1.0f, 0.0f),
        Eigen::Vector3f(0.0f, 1.0f, 0.0f),
        Eigen::Vector3f(1.0f, 1.0f, 0.0f),
        Eigen::Vector3f(1.0f, 1.0f, 0.0f),
        Eigen::Vector3f(1.0f, 0.0f, 0.0f),
        Eigen::Vector3f(1.0f, 0.0f, 0.0f),
        Eigen::Vector3f(0.0f, 0.0f, 0.0f),

        Eigen::Vector3f(0.0f, 0.0f, 1.0f),
        Eigen::Vector3f(0.0f, 1.0f, 1.0f),
        Eigen::Vector3f(0.0f, 1.0f, 1.0f),
        Eigen::Vector3f(1.0f, 1.0f, 1.0f),
        Eigen::Vector3f(1.0f, 1.0f, 1.0f),
        Eigen::Vector3f(1.0f, 0.0f, 1.0f),
        Eigen::Vector3f(1.0f, 0.0f, 1.0f),
        Eigen::Vector3f(0.0f, 0.0f, 1.0f),

        Eigen::Vector3f(0.0f, 0.0f, 0.0f),
        Eigen::Vector3f(0.0f, 0.0f, 1.0f),
        Eigen::Vector3f(0.0f, 1.0f, 0.0f),
        Eigen::Vector3f(0.0f, 1.0f, 1.0f),
        Eigen::Vector3f(1.0f, 0.0f, 0.0f),
        Eigen::Vector3f(1.0f, 0.0f, 1.0f),
        Eigen::Vector3f(1.0f, 1.0f, 0.0f),
        Eigen::Vector3f(1.0f, 1.0f, 1.0f),
    };

    glGenBuffers(1, &s_vbo_cube);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    const Eigen::Vector3f line_vertices[2] = {
        Eigen::Vector3f(0.0f, 0.0f, 0.0f),
        Eigen::Vector3f(1.0f, 1.0f, 1.0f),
    };

    glGenBuffers(1, &s_vbo_line);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo_line);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &s_vao);

    glBindVertexArray(s_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
}

void outline::shutdown(void)
{
    s_program.destroy();

    glDeleteBuffers(1, &s_vbo_cube);
    glDeleteBuffers(1, &s_vbo_line);
    glDeleteVertexArrays(1, &s_vao);
}

void outline::prepare(void)
{
    vx::throw_if_not(s_program.update());

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    auto view_projection = camera::instance.view_projection();

    glUseProgram(s_program.handle);
    glUniformMatrix4fv(s_program.uniforms[su_ViewProjection].location, 1, false, view_projection.data());
    glUniform1f(s_program.uniforms[su_ViewDistance].location, static_cast<float>(constant::CHUNK_SIZE * camera::view_distance.value()));

    glBindVertexArray(s_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
}

void outline::cube(const ChunkPos& cpos, const Eigen::Vector3f& fpos, const Eigen::Vector3f& size, float thickness,
    const Eigen::Vector4f& color)
{
    auto patch_cpos = cpos - camera::chunk;

    glLineWidth(thickness);

    auto world_position = utils::to_fvec(patch_cpos, fpos);

    glUniform3fv(s_program.uniforms[su_WorldPosition].location, 1, world_position.data());
    glUniform4fv(s_program.uniforms[su_Modulate].location, 1, color.data());
    glUniform3fv(s_program.uniforms[su_Scale].location, 1, size.data());

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo_cube);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Eigen::Vector3f), nullptr);
    glDrawArrays(GL_LINES, 0, 24);
}

void outline::line(const ChunkPos& cpos, const Eigen::Vector3f& fpos, const Eigen::Vector3f& size, float thickness,
    const Eigen::Vector4f& color)
{
    auto patch_cpos = cpos - camera::chunk;

    glLineWidth(thickness);

    auto world_position = utils::to_fvec(patch_cpos, fpos);

    glUniform3fv(s_program.uniforms[su_WorldPosition].location, 1, world_position.data());
    glUniform4fv(s_program.uniforms[su_Modulate].location, 1, color.data());
    glUniform3fv(s_program.uniforms[su_Scale].location, 1, size.data());

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo_line);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Eigen::Vector3f), nullptr);
    glDrawArrays(GL_LINES, 0, 2);
}
