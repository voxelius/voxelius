// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug_session.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/gui_screen.hh>
#include <client/message_box.hh>
#include <client/player_target.hh>
#include <client/progress_bar.hh>
#include <client/voxel_atlas.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <FastNoiseLite.h>
#include <shared/entity/head.hh>
#include <shared/entity/health.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/packet_reader.hh>
#include <shared/packet_writer.hh>
#include <shared/ray_dda.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

#include <random>

static Voxel v_slate = {};
static Voxel v_stone = {};
static Voxel v_grass = {};
static Voxel v_dirt = {};
static Voxel v_test = {};

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;

static fnl_state noise = {};

// This is VERY SLOW
// UNDONE/TODO: move this into server worldgen code
static Voxel voxel_at(const VoxelCoord &vpos)
{
    static std::uniform_int_distribution intdist = std::uniform_int_distribution(-2, +2);
    static std::mt19937_64 twister = std::mt19937_64(std::random_device()());
    int64_t surf = SURFACE + 16.0f * fnlGetNoise2D(&noise, vpos[0] / 2.0f, vpos[2] / 2.0f);
    if(vpos[1] <= surf - 32 + intdist(twister))
        return v_slate;
    if(vpos[1] <= surf - 8 + intdist(twister))
        return v_stone;
    if(vpos[1] <= surf - 1)
        return v_dirt;
    if(vpos[1] <= surf)
        return v_grass;
    return NULL_VOXEL;
}

static void generate(const ChunkCoord &cpos)
{
    spdlog::trace("generating {} {} {}", cpos[0], cpos[1], cpos[2]);

    VoxelStorage voxels = {};
    bool voxels_dirty = false;
    
    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = LocalCoord::from_index(i);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        const auto voxel = voxel_at(vpos);

        if(voxel != NULL_VOXEL) {
            voxels_dirty = true;
            voxels[i] = voxel;
            continue;
        }
    }

    if(voxels_dirty) {
        Chunk *chunk = world::find_or_create_chunk(cpos);
        chunk->voxels = voxels;
    }
}

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        if((event.action == GLFW_PRESS) && (player_target::voxel != NULL_VOXEL)) {
            if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
                world::set_voxel(NULL_VOXEL, player_target::vvec);
                return;
            }
            
            if(event.button == GLFW_MOUSE_BUTTON_RIGHT) {
                world::set_voxel(v_stone, player_target::vvec + player_target::vnormal);
                return;
            }
        }
    }
}

void debug_session::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
}

void debug_session::update(void)
{
    progress_bar::set_progress(0.5f + 0.5f * std::sin(glfwGetTime()));
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

    noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.fractal_type = FNL_FRACTAL_RIDGED;

    constexpr int WSIZE = 32;
    for(int x = -WSIZE; x < WSIZE; x += 1)
    for(int z = -WSIZE; z < WSIZE; z += 1)
    for(int y = -2; y < 1; y += 1) {
        generate({x, y, z});
    }

    constexpr int DWSIZE = 2 * WSIZE;
    for(int x = -DWSIZE; x < DWSIZE; ++x)
    for(int z = -DWSIZE; z < DWSIZE; ++z) {
        Chunk *chunk = world::find_or_create_chunk({x, -3, z});
        chunk->voxels.fill(v_stone);
    }

    Chunk *chunk = world::find_or_create_chunk({0, 4, 0});
    chunk->voxels.fill(v_test);

    spdlog::info("spawning local player");
    globals::player = globals::registry.create();
    globals::registry.emplace<PlayerComponent>(globals::player);
    globals::registry.emplace<VelocityComponent>(globals::player);

    auto &head = globals::registry.emplace<HeadComponent>(globals::player);
    auto &transform = globals::registry.emplace<TransformComponent>(globals::player);
    transform.position.local[1] += 32.0;

    progress_bar::reset();
    progress_bar::set_title("Doing something");
    progress_bar::set_button("Stop doing something", [](void) {

        PacketWriter writer = {};
        PacketWriter::string(writer, "Test packet");
        PacketWriter::FP32(writer, 1337.1337f);
        PacketWriter::UI32(writer, 42);

        PacketReader reader = {};
        PacketReader::setup(reader, writer);
        
        const auto x = PacketReader::string(reader);
        const auto y = PacketReader::FP32(reader);
        const auto z = PacketReader::UI32(reader);

        message_box::reset();
        message_box::set_title("Debug");
        message_box::set_subtitle(fmt::format("\"{}\" ({}) {} {}", x, x.size(), y, z));

        message_box::add_button("OK", [](void) {
            // Close the message box and go into the game
            globals::gui_screen = GUI_SCREEN_NONE;
        });

        message_box::add_button("Also OK", [](void) {
            // Close the message box and go into the game
            globals::gui_screen = GUI_SCREEN_NONE;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;
    });

    globals::gui_screen = GUI_PROGRESS_BAR;
}
