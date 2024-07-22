// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/util/program.hh>
#include <spdlog/spdlog.h>

GLuint util::link_program(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint info_log_length;
    std::basic_string<GLchar> info_log;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length >= 2) {
        info_log.resize(info_log_length);
        glGetProgramInfoLog(program, info_log_length, nullptr, info_log.data());
        spdlog::info("util::program: [{}; {}]: program information", vert, frag);
        spdlog::info(info_log);
    }

    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);

    if(!link_status) {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}
