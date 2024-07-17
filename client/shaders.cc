// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/shaders.hh>
#include <shared/cmdline.hh>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdio.h>
#include <unordered_map>

using module_map_t = std::unordered_map<std::string, std::string>;

static module_map_t md_vert = {};
static module_map_t md_frag = {};

constexpr static const char *common_vert = R"glsl(
    #define FRAG_SHADER 0
    #define VERT_SHADER 1
    #define M_E         2.71828182845904523536
    #define M_LOG2E     1.44269504088896340736
    #define M_LOG10E    0.43429448190325182765
    #define M_LN2       0.69314718055994530941
    #define M_LN10      2.30258509299404568402
    #define M_PI        3.14159265358979323846
    #define M_PI_2      1.57079632679489661923
    #define M_PI_4      0.78539816339744830962
    #define M_1_PI      0.31830988618379067154
    #define M_2_PI      0.63661977236758134308
    #define M_2_SQRTPI  1.12837916709551257390
    #define M_SQRT2     1.41421356237309504880
    #define M_SQRT1_2   0.70710678118654752440
)glsl";

constexpr static const char *common_frag = R"glsl(
    #define FRAG_SHADER 1
    #define VERT_SHADER 0
    #define M_E         2.71828182845904523536
    #define M_LOG2E     1.44269504088896340736
    #define M_LOG10E    0.43429448190325182765
    #define M_LN2       0.69314718055994530941
    #define M_LN10      2.30258509299404568402
    #define M_PI        3.14159265358979323846
    #define M_PI_2      1.57079632679489661923
    #define M_PI_4      0.78539816339744830962
    #define M_1_PI      0.31830988618379067154
    #define M_2_PI      0.63661977236758134308
    #define M_2_SQRTPI  1.12837916709551257390
    #define M_SQRT2     1.41421356237309504880
    #define M_SQRT1_2   0.70710678118654752440
)glsl";

constexpr static const char *screenspace_vert = R"glsl(
    const vec2 _screenspace_position[6] = {
        vec2(-1.0, -1.0), vec2(+1.0, -1.0),
        vec2(+1.0, +1.0), vec2(+1.0, +1.0),
        vec2(-1.0, +1.0), vec2(-1.0, -1.0),
    };
    const vec2 _screenspace_texcoord[6] = {
        vec2(0.0, 0.0), vec2(1.0, 0.0),
        vec2(1.0, 1.0), vec2(1.0, 1.0),
        vec2(0.0, 1.0), vec2(0.0, 0.0),
    };
    #define screenspace_position _screenspace_position[gl_VertexID % 6]
    #define screenspace_texcoord _screenspace_texcoord[gl_VertexID % 6]
)glsl";

static inline module_map_t *get_map(uint32_t stage)
{
    if(stage == GL_VERTEX_SHADER)
        return &md_vert;
    if(stage == GL_FRAGMENT_SHADER)
        return &md_frag;
    return nullptr;
}

const std::vector<std::string> split_str(const std::string &str, const std::string &sep)
{
    size_t cv = 0;
    size_t lv = 0;
    std::vector<std::string> vec;

    while((cv = str.find(sep, lv)) != std::string::npos) {
        vec.push_back(str.substr(lv, cv - lv));
        lv = cv + sep.length();
    }

    if(lv <= str.length()) {
        // We lack the last element
        vec.push_back(str.substr(lv, str.length() - lv));
    }

    return vec;
}

void shaders::init()
{
    shaders::add("common", GL_VERTEX_SHADER, common_vert);
    shaders::add("common", GL_FRAGMENT_SHADER, common_frag);
    shaders::add("screenspace", GL_VERTEX_SHADER, screenspace_vert);
}

void shaders::add(const std::string &name, uint32_t stage, const std::string &source)
{
    if(module_map_t *md = get_map(stage)) {
        const auto it = md->find(name);
        if(it != md->cend())
            return;
        md->emplace(name, source);
    }
}

bool shaders::compile(glxx::Shader &shader, const vfs::path_t &path)
{
    vfs::file_t *file = vfs::open(path, vfs::OPEN_RD);

    if(file == nullptr) {
        spdlog::error("shaders: {}: {}", path.string(), vfs::get_error());
        return false;
    }

    std::string line = {};
    std::ostringstream oss = {};

    while(vfs::read_line(file, line)) {
        char modname[128] = {};

        if(sscanf(line.c_str(), " # pragma import %127[^, \"\t\r\n]", modname) == 1) {
            if(const module_map_t *map = get_map(shader.get_stage())) {
                if(const auto it = map->find(modname); it != map->cend()) {
                    oss << fmt::format("/* begin module {} */", modname) << std::endl;
                    oss << it->second << std::endl;
                    oss << fmt::format("/* end module {} */", modname) << std::endl;
                }
                else {
                    spdlog::warn("shaders: {}: import {}: no such module", path.string(), modname);
                    oss << fmt::format("/* import {} failed: no such module */", modname);
                }
            }
        }
        else {
            // Just copy over the source file
            oss << line << std::endl;
        }
    }

    vfs::close(file);

    const std::string source = oss.str();
    const bool compiled = shader.glsl(source);

    if(!compiled || cmdline::contains("shader-debug")) {
        if(!compiled)
            spdlog::error("shaders: {}: compile failed", path.string());
        const vfs::path_t dump = fmt::format("/shaders_debug/{}", path.filename().string());
        vfs::create_directories(dump.parent_path());
        vfs::write_string(dump, source);
    }

    return compiled;
}
