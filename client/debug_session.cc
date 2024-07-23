// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug_session.hh>
#include <client/globals.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <entt/entity/registry.hpp>
#include <glm/gtc/noise.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/health.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/util/coord.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

#include <random>

static Voxel v_stone = {};
static Voxel v_grass = {};
static Voxel v_dirt = {};
static Voxel v_test = {};

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;

// This is VERY SLOW
// UNDONE/TODO: move this into server worldgen code
static Voxel voxel_at(const VoxelPos &vpos)
{
    static std::uniform_int_distribution intdist = std::uniform_int_distribution(-2, +2);
    static std::mt19937_64 twister = std::mt19937_64(std::random_device()());
    int64_t surf = SURFACE + 5.0f * glm::simplex(glm::fvec2(vpos.x, vpos.z) / 32.0f);
    if(vpos.y <= surf - 8 + intdist(twister))
        return v_stone;
    if(vpos.y <= surf - 1)
        return v_dirt;
    if(vpos.y <= surf)
        return v_grass;
    return NULL_VOXEL;
}

static void generate(const ChunkPos &cpos)
{
    spdlog::trace("generating {} {} {}", cpos.x, cpos.y, cpos.z);

    Chunk *chunk = world::find_or_create_chunk(cpos);

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = util::to_local(i);
        const auto vpos = util::to_voxel(cpos, lpos);
        const auto voxel = voxel_at(vpos);
        chunk->voxels.at(i) = voxel;
    }
}

void debug_session::run(void)
{
    if(globals::registry.valid(globals::player)) {
        // Ain't gonna run this twice
        return;
    }

    v_stone = vdef::create("stone", VoxelType::Cube).add_default_state().build();
    v_grass = vdef::create("grass", VoxelType::Cube).add_default_state().build();
    v_dirt = vdef::create("dirt", VoxelType::Cube).add_default_state().build();
    v_test = vdef::create("vtest", VoxelType::Cube).add_default_state().build();

    voxel_atlas::init(16, 16);

    for(VoxelInfo *info : vdef::voxels) {
        if(info->animated) {
            VoxelInfoAnimated *info_a = static_cast<VoxelInfoAnimated *>(info);
            for(VoxelTextureAnimated &vtex : info_a->textures) {
                for(const std::string &path : vtex.paths) {
                    if(AtlasTexture *atex = voxel_atlas::find_or_load(path, true))
                        continue;
                    spdlog::critical("voxel_atlas: {}: load failed", path);
                    std::terminate();
                }
            }
        }
        else {
            VoxelInfoVaried *info_v = static_cast<VoxelInfoVaried *>(info);
            for(VoxelTextureVaried &vtex : info_v->textures) {
                for(std::size_t i = 0; i < vtex.paths.size(); ++i) {
                    if(AtlasTexture *atex = voxel_atlas::find_or_load(vtex.paths[i], false)) {
                        vtex.indices[i] = atex->index;
                        vtex.planes[i] = atex->plane;
                        continue;
                    }

                    spdlog::critical("voxel_atlas: {}: load failed", vtex.paths[i]);
                    std::terminate();
                }
            }
        }
    }

    voxel_anims::construct();

    unsigned int w = 0U;
    for(int x = -8; x < 8; x += 2)
    for(int z = -8; z < 8; z += 2)
    for(int y = -2; y < 1; y += 1) {
        generate({x, y, z});
        //Chunk *chunk = world::find_or_create_chunk({x, y, z});
        //chunk->voxels.fill(v_stone);
    }

    Chunk *chunk = world::find_or_create_chunk({0, 1, 0});
    chunk->voxels.fill(v_test);

    spdlog::info("spawning local player");
    globals::player = globals::registry.create();
    globals::registry.emplace<PlayerComponent>(globals::player);
    globals::registry.emplace<VelocityComponent>(globals::player);

    auto &head = globals::registry.emplace<HeadComponent>(globals::player);
    head.offset = glm::dvec3{0.4, 1.4, 0.4};

    auto &transform = globals::registry.emplace<TransformComponent>(globals::player);
    transform.position.y += 16.0;

    globals::ui_screen = 0U;
}
