#include "client/pch.hh"

#include "client/shader_program.hh"

#include "core/identifier.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"

static void parse_source(std::istringstream stream, std::vector<std::string>& lines, std::vector<VariantMacro>& macros)
{
    std::string line;
    unsigned long line_number = 0UL;

    lines.clear();
    macros.clear();

    while(std::getline(stream, line)) {
        unsigned macro_index = {};
        char macro_name[128] = {};

        if(std::sscanf(line.c_str(), " # pragma variant %u %127[^, \"\t\r\n]", &macro_index, macro_name) == 2) {
            if(macros.size() <= macro_index) {
                macros.resize(macro_index + 1U);
            }

            macros[macro_index].line = line_number;
            macros[macro_index].identifier = macro_name;
            macros[macro_index].value = std::numeric_limits<unsigned>::max();

            lines.push_back(std::string());
            line_number += 1UL;
        }
        else {
            lines.push_back(line);
            line_number += 1UL;
        }
    }
}

static GLuint compile_shader(std::string_view path, const char* source, GLenum shader_stage)
{
    GLuint shader = glCreateShader(shader_stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint info_log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length >= 2) {
        std::basic_string<GLchar> info_log;
        info_log.resize(info_log_length);
        glGetShaderInfoLog(shader, info_log_length, nullptr, info_log.data());
        msg::log_info("shader information:", path);
        msg::log_info("{}", info_log);
    }

    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    if(!compile_status) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool ShaderProgram::setup(const Identifier& id)
{
    destroy();

    vert_path = id.as_file_path("shaders", ".vert");
    frag_path = id.as_file_path("shaders", ".frag");

    std::istringstream vert_stream;
    std::istringstream frag_stream;

    if(!utils::read_file(vert_path, vert_stream)) {
        msg::log_warning("{}: read failed", vert_path);
        return false;
    }

    if(!utils::read_file(frag_path, frag_stream)) {
        msg::log_warning("{}: read failed", frag_path);
        return false;
    }

    parse_source(std::move(vert_stream), vert_lines, vert_macros);
    parse_source(std::move(frag_stream), frag_lines, frag_macros);

    needs_update = true;
    handle = 0;

    return true;
}

bool ShaderProgram::update(void)
{
    if(!needs_update) {
        return true;
    }

    for(const auto& macro : vert_macros) {
        vert_lines[macro.line] = std::format("#define {} {}", macro.identifier, macro.value);
    }

    for(const auto& macro : frag_macros) {
        frag_lines[macro.line] = std::format("#define {} {}", macro.identifier, macro.value);
    }

    auto vert_source = utils::join(vert_lines, "\r\n");
    auto frag_source = utils::join(frag_lines, "\r\n");

    auto vert = compile_shader(vert_path, vert_source.c_str(), GL_VERTEX_SHADER);
    auto frag = compile_shader(frag_path, frag_source.c_str(), GL_FRAGMENT_SHADER);

    if(vert == 0 || frag == 0) {
        glDeleteShader(frag);
        glDeleteShader(vert);
        return false;
    }

    handle = glCreateProgram();
    glAttachShader(handle, vert);
    glAttachShader(handle, frag);
    glLinkProgram(handle);

    GLint info_log_length;
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length >= 2) {
        std::basic_string<GLchar> info_log;
        info_log.resize(info_log_length);
        glGetProgramInfoLog(handle, info_log_length, nullptr, info_log.data());
        msg::log_info("program information:");
        msg::log_info("{}", info_log);
    }

    glDeleteShader(frag);
    glDeleteShader(vert);

    GLint link_status;
    glGetProgramiv(handle, GL_LINK_STATUS, &link_status);

    if(!link_status) {
        glDeleteProgram(handle);
        return false;
    }

    for(auto& uniform : uniforms) {
        // NOTE: OpenGL seems to silently ignore invalid uniform
        // locations (-1); should we write something into logs about this?
        uniform.location = glGetUniformLocation(handle, uniform.identifier.c_str());
    }

    needs_update = false;
    return true;
}

void ShaderProgram::destroy(void)
{
    if(handle) {
        glDeleteProgram(handle);
        handle = 0;
    }

    uniforms.clear();

    frag_macros.clear();
    frag_lines.clear();
    frag_path = std::string();

    vert_macros.clear();
    vert_lines.clear();
    vert_path = std::string();

    needs_update = false;
}

std::size_t ShaderProgram::add_uniform(std::string name)
{
    for(std::size_t i = 0; i < uniforms.size(); ++i) {
        if(0 == uniforms[i].identifier.compare(name)) {
            return i;
        }
    }

    const std::size_t index = uniforms.size();
    uniforms.push_back(VariantUniform());
    uniforms[index].identifier = std::move(name);
    uniforms[index].location = -1;
    return index;
}

void ShaderProgram::set_variant_vert(unsigned variant, unsigned value)
{
    if(variant < vert_macros.size()) {
        if(value != vert_macros[variant].value) {
            vert_macros[variant].value = value;
            needs_update = true;
        }
    }
}

void ShaderProgram::set_variant_frag(unsigned variant, unsigned value)
{
    if(variant < frag_macros.size()) {
        if(value != frag_macros[variant].value) {
            frag_macros[variant].value = value;
            needs_update = true;
        }
    }
}
