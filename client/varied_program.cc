// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <cstdio>
#include <client/varied_program.hh>
#include <shared/fstools.hh>
#include <shared/strtools.hh>
#include <spdlog/spdlog.h>
#include <sstream>

// This fills up the array of source lines and figures out
// which lines are to be dynamically resolved as variant macros
static void parse_source(const std::string &source, std::vector<std::string> &out_lines, std::vector<VariedMacro> &out_variants)
{
    std::string line = {};
    std::istringstream stream = std::istringstream(source);
    unsigned long line_number = 0UL;
    
    out_lines.clear();
    out_variants.clear();
    
    while(std::getline(stream, line)) {
        unsigned int macro_index = {};
        char macro_name[128] = {};
        
        if(std::sscanf(line.c_str(), " # pragma variant [ %u ] %127[^, \"\t\r\n]", &macro_index, &macro_name) == 2) {
            if(out_variants.size() <= macro_index)
                out_variants.resize(macro_index + 1U);
            out_variants[macro_index].name = macro_name;
            out_variants[macro_index].line = line_number;
            out_variants[macro_index].value = std::numeric_limits<unsigned int>::max();

            out_lines.push_back(std::string());
            line_number += 1UL;
        }
        else {
            out_lines.push_back(line);
            line_number += 1UL;
        }
    }
}

static GLuint compile_shader(const std::string &path, const std::string &source, GLenum shader_stage)
{
    const char *source_cstr = source.c_str();
    GLuint shader = glCreateShader(shader_stage);
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    GLint info_log_length;
    std::basic_string<GLchar> info_log = {};
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length >= 2) {
        info_log.resize(info_log_length);
        glGetShaderInfoLog(shader, info_log_length, nullptr, info_log.data());
        spdlog::info("varied_program: {}: shader information:", path);
        spdlog::info(info_log);
    }

    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    if(!compile_status) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool VariedProgram::setup(VariedProgram &program, const std::string &vert_path, const std::string &frag_path)
{
    std::string vert_source = {};
    std::string frag_source = {};

    VariedProgram::destroy(program);

    program.vert_path = vert_path;
    program.frag_path = frag_path;

    if(!fstools::read_string(program.vert_path, vert_source)) {
        spdlog::warn("varied_program: {}: {}", program.vert_path, fstools::error());
        return false;
    }

    if(!fstools::read_string(program.frag_path, frag_source)) {
        spdlog::warn("varied_program: {}: {}", program.frag_path, fstools::error());
        return false;
    }

    parse_source(vert_source, program.vert_source, program.vert_variants);
    parse_source(frag_source, program.frag_source, program.frag_variants);

    program.needs_update = true;
    program.handle = 0;

    return true;
}

bool VariedProgram::update(VariedProgram &program)
{
    if(!program.needs_update) {
        // The program is already up to
        // date with the internal state
        return true;
    }

    for(const VariedMacro &macro : program.vert_variants)
        program.vert_source[macro.line] = fmt::format("#define {} {}", macro.name, macro.value);
    for(const VariedMacro &macro : program.frag_variants)
        program.frag_source[macro.line] = fmt::format("#define {} {}", macro.name, macro.value);

    const std::string vert_source = strtools::join(program.vert_source, "\r\n");
    const std::string frag_source = strtools::join(program.frag_source, "\r\n");

    GLuint vert = compile_shader(program.vert_path, vert_source, GL_VERTEX_SHADER);
    GLuint frag = compile_shader(program.frag_path, frag_source, GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        //program.needs_update = false;
        glDeleteShader(frag);
        glDeleteShader(vert);
        return false;
    }

    program.handle = glCreateProgram();
    glAttachShader(program.handle, vert);
    glAttachShader(program.handle, frag);
    glLinkProgram(program.handle);

    GLint info_log_length;
    std::basic_string<GLchar> info_log;
    glGetProgramiv(program.handle, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length >= 2) {
        info_log.resize(info_log_length);
        glGetProgramInfoLog(program.handle, info_log_length, nullptr, info_log.data());
        spdlog::info("varied_program: [{}; {}]: program information", vert, frag);
        spdlog::info(info_log);
    }

    glDeleteShader(frag);
    glDeleteShader(vert);

    GLint link_status;
    glGetProgramiv(program.handle, GL_LINK_STATUS, &link_status);

    if(!link_status) {
        //program.needs_update = false;
        glDeleteProgram(program.handle);
        return false;
    }

    for(VariedUniform &uniform : program.uniforms) {
        // NOTE: GL seems to silently ignore invalid uniform
        // locations (-1); should we write something into logs about this?
        uniform.location = glGetUniformLocation(program.handle, uniform.name.c_str());
    }

    program.needs_update = false;
    return true;
}

void VariedProgram::destroy(VariedProgram &program)
{
    if(program.handle)
        glDeleteProgram(program.handle);
    program.handle = 0;

    program.uniforms.clear();

    program.frag_variants.clear();
    program.frag_source.clear();
    program.frag_path = std::string();

    program.vert_variants.clear();
    program.vert_source.clear();
    program.vert_path = std::string();

    program.needs_update = false;
}

std::size_t VariedProgram::add_uniform(VariedProgram &program, const std::string &name)
{
    for(std::size_t i = 0; i < program.uniforms.size(); ++i) {
        if(program.uniforms[i].name.compare(name))
            continue;
        return i;
    }
    
    const std::size_t index = program.uniforms.size();
    program.uniforms.push_back(VariedUniform());
    program.uniforms[index].location = -1;
    program.uniforms[index].name = name;
    return index;
}

void VariedProgram::set_variant_vert(VariedProgram &program, unsigned int variant, unsigned int value)
{
    if(variant < program.vert_variants.size()) {
        if(value != program.vert_variants[variant].value) {
            program.vert_variants[variant].value = value;
            program.needs_update = true;
        }
    }
    
}

void VariedProgram::set_variant_frag(VariedProgram &program, unsigned int variant, unsigned int value)
{
    if(variant < program.frag_variants.size()) {
        if(value != program.frag_variants[variant].value) {
            program.frag_variants[variant].value = value;
            program.needs_update = true;
        }
    }
}
