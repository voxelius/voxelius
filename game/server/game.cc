// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <entt/entity/registry.hpp>
#include <game/server/game.hh>
#include <game/server/globals.hh>
#include <game/server/receive.hh>
#include <game/server/sessions.hh>
#include <game/server/status.hh>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/player.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/game_voxels.hh>
#include <game/shared/protocol.hh>
#include <game/shared/splash.hh>
#include <mathlib/constexpr.hh>
#include <spdlog/spdlog.h>

// UNDONE: worldgen in shared
#include <FastNoiseLite.h>
#include <game/shared/world.hh>
#include <random>

static unsigned int listen_port = protocol::PORT;
static unsigned int status_peers = 4U;

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
        return game_voxels::slate;
    if(vpos[1] <= surf - 8 + intdist(twister))
        return game_voxels::stone;
    if(vpos[1] <= surf - 1)
        return game_voxels::dirt;
    if(vpos[1] <= surf)
        return game_voxels::grass;
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
        Chunk *chunk = world::assign(cpos, globals::registry.create());
        chunk->voxels = voxels;
    }
}

void server_game::init(void)
{
    Config::add(globals::server_config, "game.listen_port", listen_port);
    Config::add(globals::server_config, "game.status_peers", status_peers);

    sessions::init();

    splash::init("texts/motds.txt");
    status::init();

    server_recieve::init();

    world::init();
}

void server_game::init_late(void)
{
    sessions::init_late();

    listen_port = cxpr::clamp<unsigned int>(listen_port, 1024U, UINT16_MAX);
    status_peers = cxpr::clamp<unsigned int>(status_peers, 2U, 16U);

    ENetAddress address = {};
    address.host = ENET_HOST_ANY;
    address.port = listen_port;

    globals::server_host = enet_host_create(&address, sessions::max_players + status_peers, 1, 0, 0);

    if(!globals::server_host) {
        spdlog::critical("game: unable to setup an ENet host");
        std::terminate();
    }

    spdlog::info("game: host: {} player + {} status peers", sessions::max_players, status_peers);
    spdlog::info("game: host: listening on UDP port {}", address.port);

    game_voxels::populate();

    // Debug

    noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.fractal_type = FNL_FRACTAL_RIDGED;

    constexpr int WSIZE = 8;
    for(int x = -WSIZE; x < WSIZE; x += 1)
    for(int z = -WSIZE; z < WSIZE; z += 1)
    for(int y = -2; y < 1; y += 1) {
        generate({x, y, z});
    }

    constexpr int DWSIZE = 2 * WSIZE;
    for(int x = -DWSIZE; x < DWSIZE; ++x)
    for(int z = -DWSIZE; z < DWSIZE; ++z) {
        const auto pos = ChunkCoord(x, -3, z);
        Chunk *chunk = world::find(pos);
        if(!chunk)
            chunk = world::assign(pos, globals::registry.create());
        chunk->voxels.fill(game_voxels::stone);
    }

    const auto pos = ChunkCoord(0, 4, 0);
    Chunk *chunk = world::find(pos);
    if(!chunk)
        chunk = world::assign(pos, globals::registry.create());
    chunk->voxels.fill(game_voxels::vtest);
}

void server_game::deinit(void)
{
    protocol::send_disconnect(nullptr, globals::server_host, "protocol.server_shutdown");

    sessions::deinit();

    enet_host_flush(globals::server_host);
    enet_host_service(globals::server_host, nullptr, 500);
    enet_host_destroy(globals::server_host);
}

void server_game::update(void)
{
#if 0
    const auto group = globals::registry.group<PlayerComponent, HeadComponent, TransformComponent, VelocityComponent>();

    for(const auto [entity, head, transform, velocity] : group.each()) {
        spdlog::info("player: {} {} {} / {} {} {}",
            transform.position.chunk[0], transform.position.chunk[1], transform.position.chunk[2],
            transform.position.local[0], transform.position.local[1], transform.position.local[2]);
    }
#endif
}

void server_game::update_late(void)
{    
    ENetEvent event = {};

    while(enet_host_service(globals::server_host, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
            sessions::destroy(sessions::find(event.peer));
            continue;
        }

        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            protocol::receive(event.packet, event.peer);
            enet_packet_destroy(event.packet);
            continue;
        }
    }
}
