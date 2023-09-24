// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <cstdio> // for sscanf
#include <client/shaders.hh>
#include <shared/cmdline.hh>
#include <shared/filetools.hh>
#include <spdlog/spdlog.h>
#include <sstream>

// Shader modules
static std::unordered_map<Identifier, shader_module_t> modules;

// Module: voxelius:common
// Description: common shading language extensions
static const shader_module_t common_module = {
    {GL_FRAGMENT_SHADER, R"glsl(
        #define FRAG_SHADER 1
        #define VERT_SHADER 0
        #define inout in
    )glsl"},
    {GL_VERTEX_SHADER, R"glsl(
        #define FRAG_SHADER 0
        #define VERT_SHADER 1
        #define inout out
        out gl_PerVertex {
            vec4 gl_Position;
            float gl_PointSize;
            float gl_ClipDistance[];
        };
    )glsl"},
};

// Module: voxelius:screenspace
// Description: screenspace shader functionality
static const shader_module_t screenspace_module = {
    {GL_VERTEX_SHADER, R"glsl(
        const vec2 _screenspace_position[6] = {
            vec2(-1.0, -1.0), vec2(-1.0,  1.0),
            vec2( 1.0,  1.0), vec2( 1.0,  1.0),
            vec2( 1.0, -1.0), vec2(-1.0, -1.0),
        };
        const vec2 _screenspace_texcoord[6] = {
            vec2(0.0, 0.0), vec2(0.0, 1.0),
            vec2(1.0, 1.0), vec2(1.0, 1.0),
            vec2(1.0, 0.0), vec2(0.0, 0.0),
        };
        #define screenspace_position _screenspace_position[gl_VertexID % 6]
        #define screenspace_texcoord _screenspace_texcoord[gl_VertexID % 6]
    )glsl"},
};

void shaders::init()
{
    shaders::add_module(Identifier{Identifier::DEFAULT_NAMESPACE, "common"}, common_module);
    shaders::add_module(Identifier{Identifier::DEFAULT_NAMESPACE, "screenspace"}, screenspace_module);
}

void shaders::add_module(const Identifier &name, const shader_module_t &md)
{
    const auto it = modules.find(name);
    if(it != modules.cend())
        spdlog::warn("shaders: module {} is present and will be overwritten!", name.get());
    modules[name] = md;
}

bool shaders::compile(glxx::Shader &shader, const Identifier &path)
{
    PHYSFS_File *file = PHYSFS_openRead(path.get_file_path("shaders").c_str());

    if(!file) {
        spdlog::warn("shaders: cannot compile {}: {}", path.get(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return false;
    }

    std::string line;
    std::stringstream ss;
    while(util::readline(file, line)) {
        char modname[128] = {0};
        if(sscanf(line.c_str(), " # import %127[^, \"\t\r\n]", modname) == 1) {
            const auto module_it = modules.find(Identifier{modname});
            if(module_it != modules.cend()) {
                const auto stage_it = module_it->second.find(shader.get_stage());
                if(stage_it != module_it->second.cend()) {
                    ss << "// begin module " << modname << std::endl;
                    ss << stage_it->second << std::endl;
                    ss << "// end module " << modname << std::endl;
                    continue;
                }

                ss << "// import " << modname << " failed: no stage" << std::endl;
                continue;
            }

            ss << "// import " << modname << " failed: no module" << std::endl;
            continue;
        }

        ss << line << std::endl;
    }

    PHYSFS_close(file);

    const std::string source = ss.str();
    bool compiled = shader.glsl(source);

    if(!compiled || cmdline::contains("shader-debug")) {
        if(!compiled)
            spdlog::error("shaders: cannot compile {}", path.get());
        spdlog::debug("{}: {}", path.get(), source);
    }

    return compiled;
}
