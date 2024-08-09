// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VARIED_PROGRAM_HH
#define CLIENT_VARIED_PROGRAM_HH
#include <array>
#include <glad/gl.h>
#include <string>
#include <vector>

struct VariedMacro final {
    std::string name {};
    unsigned long line {};
    unsigned int value {};
};

struct VariedUniform final {
    std::string name {};
    GLint location {};
};

class VariedProgram final {
public:
    std::string vert_path {};
    std::string frag_path {};
    std::vector<std::string> vert_source {};
    std::vector<std::string> frag_source {};
    std::vector<VariedMacro> vert_variants {};
    std::vector<VariedMacro> frag_variants {};
    std::vector<VariedUniform> uniforms {};
    bool needs_update {};
    GLuint handle {};

public:
    static bool setup(VariedProgram &program, const std::string &vert_path, const std::string &frag_path);
    static bool update(VariedProgram &program);
    static void destroy(VariedProgram &program);

public:
    static std::size_t uniform(VariedProgram &program, const std::string &name);
    static void variant_vert(VariedProgram &program, unsigned int variant, unsigned int value);
    static void variant_frag(VariedProgram &program, unsigned int variant, unsigned int value);
};

#endif /* CLIENT_VARIED_PROGRAM_HH */
