// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/debug_session.hh>
#include <client/globals.hh>
#include <client/ui_screen.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <entt/entity/registry.hpp>
#include <glm/gtc/noise.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

#include <random>

static Voxel STONE;
static Voxel SLATE;
static Voxel DIRT;
static Voxel GRASS;
static Voxel VTEST;

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;

// This is VERY SLOW
// UNDONE/TODO: move this into server worldgen code
static Voxel voxel_at(const VoxelCoord &vv)
{
    static std::uniform_int_distribution intdist = std::uniform_int_distribution(-2, +2);
    static std::mt19937_64 twister = std::mt19937_64(std::random_device()());
    int64_t surf = SURFACE + 4.0 * glm::simplex(glm::dvec2{vv.x, vv.z} / 48.0);
    if(vv.y <= surf - 32 + intdist(twister))
        return SLATE;
    if(vv.y <= surf - 8 + intdist(twister))
        return STONE;
    if(vv.y <= surf - 1)
        return DIRT;
    if(vv.y <= surf)
        return GRASS;
    return NULL_VOXEL;
}

static void generate(const ChunkCoord &cv)
{
    spdlog::trace("generating {} {} {}", cv.x, cv.y, cv.z);

    Chunk *chunk = world::find_or_create_chunk(cv);

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lv = coord::to_local(i);
        const auto vv = coord::to_voxel(cv, lv);
        const auto voxel = voxel_at(vv);
        voxel_storage::set(chunk->storage, voxel, i);
    }
}

void debug_session::run()
{
    if(!globals::registry.valid(globals::player)) {
        STONE   = vdef::create(VOXEL_TYPE_CUBE, "stone").state_default().build();
        SLATE   = vdef::create(VOXEL_TYPE_CUBE, "slate").state_default().build();
        DIRT    = vdef::create(VOXEL_TYPE_CUBE, "dirt").state_default().build();
        GRASS   = vdef::create(VOXEL_TYPE_CUBE, "grass").state_default().build();
        VTEST   = vdef::create(VOXEL_TYPE_CUBE, "vtest").state_default().build();

        voxel_atlas::create(16, 16, vdef::textures.size());

        for(const vfs::path_t &path : vdef::textures) {
            if(!voxel_atlas::load(path)) {
                spdlog::critical("atlas: {}: load failed", path.string());
                std::terminate();
            }
        }

        voxel_anims::construct();

        unsigned int w = 0U;
        for(int x = -4; x < 4; ++x) {
            for(int z = -4; z < 4; ++z) {
                for(int y = -2; y < 1; ++y) {
                    generate({x, y, z});
                    //Chunk *chunk = world::find_or_create_chunk({x, y, z});
                    //voxel_storage::fill(chunk->storage, STONE);
                }
            }
        }

        spdlog::info("spawning local player");
        globals::player = globals::registry.create();
        globals::registry.emplace<PlayerComponent>(globals::player);
        globals::registry.emplace<VelocityComponent>(globals::player);

        auto &head = globals::registry.emplace<HeadComponent>(globals::player);
        head.offset = glm::dvec3{0.4, 1.4, 0.4};

        auto &transform = globals::registry.emplace<TransformComponent>(globals::player);
        transform.position.y += 16.0;

        globals::ui_screen = ui::SCREEN_NONE;
    }
}
