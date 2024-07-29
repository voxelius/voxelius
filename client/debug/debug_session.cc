// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug/debug_session.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/world/voxel_atlas.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/health.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/world/ray_dda.hh>
#include <shared/world/vdef.hh>
#include <shared/world/world.hh>
#include <spdlog/spdlog.h>

#include <random>

static Voxel v_slate = {};
static Voxel v_stone = {};
static Voxel v_grass = {};
static Voxel v_dirt = {};
static Voxel v_test = {};

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;

#if 0
static float octanoise(const glm::fvec2 &vec, int count)
{
    float result = 0.0f;
    for(int i = 1; i <= count; ++i)
        result += glm::simplex(vec * static_cast<float>(i)) / static_cast<float>(i);
    return result;
}

// This is VERY SLOW
// UNDONE/TODO: move this into server worldgen code
static Voxel voxel_at(const VoxelCoord &vpos)
{
    static std::uniform_int_distribution intdist = std::uniform_int_distribution(-2, +2);
    static std::mt19937_64 twister = std::mt19937_64(std::random_device()());
    int64_t surf = SURFACE + 3.0f * octanoise(glm::fvec2(vpos.x, vpos.z) / 64.0f, 4);
    if(vpos.y <= surf - 32 + intdist(twister))
        return v_slate;
    if(vpos.y <= surf - 8 + intdist(twister))
        return v_stone;
    if(vpos.y <= surf - 1)
        return v_dirt;
    if(vpos.y <= surf)
        return v_grass;
    return NULL_VOXEL;
}

static void generate(const ChunkCoord &cpos)
{
    spdlog::trace("generating {} {} {}", cpos.x, cpos.y, cpos.z);

    Chunk *chunk = world::find_or_create_chunk(cpos);

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = coord::to_local(i);
        const auto vpos = coord::to_voxel(cpos, lpos);
        const auto voxel = voxel_at(vpos);

        if(voxel != NULL_VOXEL) {
            chunk->voxels[i] = voxel;
            continue;
        }
    }
}
#endif

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{
    if(!globals::ui_screen && globals::registry.valid(globals::player)) {
        if(event.action == GLFW_PRESS) {
            RayDDA ray = {};
            RayDDA::setup(ray, view::position, view::direction);

            do {
                if(RayDDA::step(ray) != NULL_VOXEL) {
                    if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
                        world::set_voxel(NULL_VOXEL, ray.vpos);
                        return;
                    }

                    if(event.button == GLFW_MOUSE_BUTTON_RIGHT) {
                        world::set_voxel(v_stone, ray.vpos + ray.vnormal);
                        return;
                    }

                    break;
                }
            } while(ray.distance < 16.0f);
        }
    }
}

void debug_session::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
}

void debug_session::update(void)
{

}

void debug_session::run(void)
{
    if(globals::registry.valid(globals::player)) {
        // Ain't gonna run this twice
        return;
    }

    v_slate = vdef::create("slate", VoxelType::Cube).add_default_state().build();
    v_stone = vdef::create("stone", VoxelType::Cube).add_default_state().build();
    v_grass = vdef::create("grass", VoxelType::Cube).add_default_state().build();
    v_dirt = vdef::create("dirt", VoxelType::Cube).add_default_state().build();
    v_test = vdef::create("vtest", VoxelType::Cube).add_default_state().build();

    // Figure out the total texture count
    // NOTE: this is debug, early and a very
    // conservative limit choice. There must be a way
    // to make it less shit if we talk about memory conservation
    std::size_t max_texture_count = 0;

    for(const VoxelInfo &info : vdef::voxels) {
        for(const VoxelTexture &vtex : info.textures) {
            max_texture_count += vtex.paths.size();
        }
    }

    voxel_atlas::create(16, 16, max_texture_count);

    // Add cached strip values to the VoxelTexture objects
    for(VoxelInfo &info : vdef::voxels) {
        for(VoxelTexture &vtex : info.textures) {
            if(AtlasStrip *strip = voxel_atlas::find_or_load(vtex.paths)) {
                vtex.cached_offset = strip->offset;
                vtex.cached_plane = strip->plane;
                continue;
            }
            
            spdlog::critical("debug_session: {}: failed to load atlas strips", info.name);
            std::terminate();
        }
    }

    voxel_atlas::generate_mipmaps();


#if 0
    constexpr int WSIZE = 8;
    constexpr int WHEIGHT = 1;
    unsigned int w = 0U;
    for(int x = -WSIZE; x < WSIZE; x += 1)
    for(int z = -WSIZE; z < WSIZE; z += 1)
    for(int y = -2; y < WHEIGHT; y += 1) {
        generate({x, y, z});
        //Chunk *chunk = world::find_or_create_chunk({x, y, z});
        //chunk->voxels.fill(v_stone);
    }
#endif

    for(int i = -8; i < 8; i += 2)
    for(int j = -8; j < 8; j += 2) {
        Chunk *chunk = world::find_or_create_chunk({i, 0, j});
        chunk->voxels.fill(v_test);
    }

    spdlog::info("spawning local player");
    globals::player = globals::registry.create();
    globals::registry.emplace<PlayerComponent>(globals::player);
    globals::registry.emplace<VelocityComponent>(globals::player);

    auto &head = globals::registry.emplace<HeadComponent>(globals::player);
    auto &transform = globals::registry.emplace<TransformComponent>(globals::player);
    //transform.position.local[1] += 16.0;

    globals::ui_screen = 0U;
}
