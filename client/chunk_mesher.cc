// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/chunk_mesh.hh>
#include <client/atlas.hh>
#include <client/chunk_mesher.hh>
#include <client/globals.hh>
#include <client/quad_vertex.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/entity/chunk.hh>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/voxel_set.hh>
#include <shared/util/coord.hh>
#include <shared/util/crc64.hh>
#include <shared/const.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>
#include <thread_pool.hpp>

using QuadBuilder = std::vector<QuadVertex>;

using CachedChunkPos = unsigned short;
constexpr static CachedChunkPos CPOS_ITSELF = 0x0000;
constexpr static CachedChunkPos CPOS_NORTH  = 0x0001;
constexpr static CachedChunkPos CPOS_SOUTH  = 0x0002;
constexpr static CachedChunkPos CPOS_EAST   = 0x0003;
constexpr static CachedChunkPos CPOS_WEST   = 0x0004;
constexpr static CachedChunkPos CPOS_TOP    = 0x0005;
constexpr static CachedChunkPos CPOS_BOTTOM = 0x0006;
constexpr static const size_t NUM_CACHED_CPOS = 7;

struct WorkerContext final {
    std::array<std::unique_ptr<VoxelArray>, NUM_CACHED_CPOS> cache {};
    std::vector<QuadBuilder> quads {};
    std::shared_future<bool> future {};
    bool is_cancelled {};
    ChunkPos coord {};
};

static const CachedChunkPos get_cached_cpos(const ChunkPos &pivot, const ChunkPos &cpos)
{
    static const CachedChunkPos nx[3] = {CPOS_WEST, 0, CPOS_EAST};
    static const CachedChunkPos ny[3] = {CPOS_BOTTOM, 0, CPOS_TOP};
    static const CachedChunkPos nz[3] = {CPOS_NORTH, 0, CPOS_SOUTH};

    if(pivot != cpos) {
        ChunkPos delta = (pivot - cpos);
        delta.x = util::clamp(delta.x, -1, 1);
        delta.y = util::clamp(delta.y, -1, 1);
        delta.z = util::clamp(delta.z, -1, 1);

        if(delta.x)
            return nx[delta.x + 1];
        if(delta.y)
            return ny[delta.y + 1];
        return nz[delta.z + 1];
    }

    return CPOS_ITSELF;
}

static bool vis_test(WorkerContext *ctx, Voxel voxel, const VoxelInfo *info, const LocalPos &lpos)
{
    const auto pvpos = util::to_voxel(ctx->coord, lpos);
    const auto pcpos = util::to_chunk(pvpos);
    const auto plpos = util::to_local(pvpos);
    const auto index = util::to_index(plpos);

    const auto cached_cpos = get_cached_cpos(ctx->coord, pcpos);
    const auto &voxels = ctx->cache.at(cached_cpos);
    const auto neighbour = voxels ? voxels->at(index) : NULL_VOXEL;

    if(neighbour == NULL_VOXEL)
        return true;
    if(neighbour == voxel)
        return false;

    if(const VoxelInfo *neighbour_info = vdef::find(neighbour)) {
        if(neighbour_info->blending != info->blending) {
            // Voxel types that use blending are semi-transparent;
            // this means they're rendered using a different setup
            // and they must have visible faces with opaque voxels
            return true;
        }
    }

    return false;
}

static VoxelFacing get_facing(VoxelFace face, VoxelType type)
{
    if(type == VoxelType::Cross) {
        switch(face) {
            case VoxelFace::CrossNESW:  return FACING_NESW;
            case VoxelFace::CrossNWSE:  return FACING_NWSE;
            default:                    return FACING_NORTH;
        }
    }

    switch(face) {
        case VoxelFace::CubeNorth:  return FACING_NORTH;
        case VoxelFace::CubeSouth:  return FACING_SOUTH;
        case VoxelFace::CubeEast:   return FACING_EAST;
        case VoxelFace::CubeWest:   return FACING_WEST;
        case VoxelFace::CubeTop:    return FACING_UP;
        case VoxelFace::CubeBottom: return FACING_DOWN;
        default:                    return FACING_NORTH;
    }
}

static void push_quad_a(WorkerContext *ctx, const VoxelInfo *info, const glm::fvec3 &pos, const glm::fvec2 &size, VoxelFace face)
{
    const VoxelFacing facing = get_facing(face, info->type);
    const VoxelTexture &vtex = info->textures[static_cast<std::size_t>(face)];
    ctx->quads[vtex.cached_plane].push_back(make_quad_vertex(pos, size, facing, vtex.cached_offset, vtex.paths.size()));
}

static void push_quad_v(WorkerContext *ctx, const VoxelInfo *info, const glm::fvec3 &pos, const glm::fvec2 &size, VoxelFace face, std::size_t entropy)
{
    const VoxelFacing facing = get_facing(face, info->type);
    const VoxelTexture &vtex = info->textures[static_cast<std::size_t>(face)];
    const std::size_t entropy_mod = entropy % vtex.paths.size();
    ctx->quads[vtex.cached_plane].push_back(make_quad_vertex(pos, size, facing, vtex.cached_offset + entropy_mod, 0));
}

static void make_cube(WorkerContext *ctx, Voxel voxel, const VoxelInfo *info, const LocalPos &lpos, VoxelVis vis, std::size_t entropy)
{
    const glm::fvec3 fpos = glm::fvec3(lpos);
    const glm::fvec2 fsize = glm::fvec2(1.0f, 1.0f);

    if(info->animated) {
        if(vis & VIS_NORTH) push_quad_a(ctx, info, fpos, fsize, VoxelFace::CubeNorth);
        if(vis & VIS_SOUTH) push_quad_a(ctx, info, fpos, fsize, VoxelFace::CubeSouth);
        if(vis & VIS_EAST)  push_quad_a(ctx, info, fpos, fsize, VoxelFace::CubeEast);
        if(vis & VIS_WEST)  push_quad_a(ctx, info, fpos, fsize, VoxelFace::CubeWest);
        if(vis & VIS_UP)    push_quad_a(ctx, info, fpos, fsize, VoxelFace::CubeTop);
        if(vis & VIS_DOWN)  push_quad_a(ctx, info, fpos, fsize, VoxelFace::CubeBottom);
    }
    else {
        if(vis & VIS_NORTH) push_quad_v(ctx, info, fpos, fsize, VoxelFace::CubeNorth, entropy);
        if(vis & VIS_SOUTH) push_quad_v(ctx, info, fpos, fsize, VoxelFace::CubeSouth, entropy);
        if(vis & VIS_EAST)  push_quad_v(ctx, info, fpos, fsize, VoxelFace::CubeEast, entropy);
        if(vis & VIS_WEST)  push_quad_v(ctx, info, fpos, fsize, VoxelFace::CubeWest, entropy);
        if(vis & VIS_UP)    push_quad_v(ctx, info, fpos, fsize, VoxelFace::CubeTop, entropy);
        if(vis & VIS_DOWN)  push_quad_v(ctx, info, fpos, fsize, VoxelFace::CubeBottom, entropy);
    }
}

static void cache_chunk(WorkerContext *ctx, const ChunkPos &cpos)
{
    const auto index = get_cached_cpos(ctx->coord, cpos);
    if(const Chunk *chunk = world::find_chunk(cpos))
        ctx->cache[index] = std::make_unique<VoxelArray>(chunk->voxels);
    else ctx->cache[index] = nullptr;
}

static void process(WorkerContext *ctx)
{
    ctx->quads.resize(atlas::plane_count());

    const std::unique_ptr<VoxelArray> &voxels = ctx->cache.at(CPOS_ITSELF);

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        if(ctx->is_cancelled) {
            ctx->quads.clear();
            return;
        }

        const auto voxel = voxels->at(i);
        const auto lpos = util::to_local(i);

        const VoxelInfo *info = vdef::find(voxel);

        if(info == nullptr) {
            // Either a NULL_VOXEL or something went
            // horribly wrong and we don't what this is
            continue;
        }

        VoxelVis vis = 0;
        if(vis_test(ctx, voxel, info, lpos + LocalPos(WDIR_NORTH)))
            vis |= VIS_NORTH;
        if(vis_test(ctx, voxel, info, lpos + LocalPos(WDIR_SOUTH)))
            vis |= VIS_SOUTH;
        if(vis_test(ctx, voxel, info, lpos + LocalPos(WDIR_EAST)))
            vis |= VIS_EAST;
        if(vis_test(ctx, voxel, info, lpos + LocalPos(WDIR_WEST)))
            vis |= VIS_WEST;
        if(vis_test(ctx, voxel, info, lpos + LocalPos(WDIR_UP)))
            vis |= VIS_UP;
        if(vis_test(ctx, voxel, info, lpos + LocalPos(WDIR_DOWN)))
            vis |= VIS_DOWN;

        const VoxelPos vpos = util::to_voxel(ctx->coord, lpos);
        const VoxelPos::value_type entropy_src = vpos.x * vpos.y * vpos.z;
        const auto entropy = util::crc64(&entropy_src, sizeof(entropy_src));

        // FIXME: handle different voxel types
        make_cube(ctx, voxel, info, lpos, vis, entropy);
    }
}

static void finalize(WorkerContext *ctx, entt::entity entity)
{
    auto &comp = globals::registry.emplace_or_replace<ChunkMeshComponent>(entity);
    const std::size_t plane_count = ctx->quads.size();

    comp.quad.resize(plane_count);

    for(std::size_t plane = 0; plane < plane_count; ++plane) {
        QuadBuilder &builder = ctx->quads[plane];
        VertexBuffer &buffer = comp.quad[plane];

        if(builder.empty()) {
            if(buffer.handle) {
                glDeleteBuffers(1, &buffer.handle);
                buffer.handle = 0;
                buffer.size = 0;
            }
        }
        else {
            if(!buffer.handle)
                glGenBuffers(1, &buffer.handle);
            glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
            glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * builder.size(), builder.data(), GL_STATIC_DRAW);
            buffer.size = builder.size();
        }
    }
}

#if defined(_WIN32) && !defined(NDEBUG)
// Debug builds for Win32 have a distinctive feature
// of utterly shitting themselves during threaded meshing.
// The code generated by MSVC on Debug configuration is
// just so slow and full of whatever debug shit the compiler
// decides to put there, it is slower than generating terrain.
constexpr static const size_t MESHER_THREADS_COUNT = 1;
constexpr static const size_t MESHER_TASKS_PER_FRAME = 16;
#else
constexpr static const size_t MESHER_THREADS_COUNT = 2;
constexpr static const size_t MESHER_TASKS_PER_FRAME = 32;
#endif

static thread_pool workers_pool = thread_pool(MESHER_THREADS_COUNT);
static std::unordered_map<ChunkPos, std::unique_ptr<WorkerContext>> workers = {};

// Bogus internal flag component
struct NeedsMeshingComponent final {};

static void on_chunk_create(const ChunkCreateEvent &event)
{
    const std::array<ChunkPos, 6> neighbours = {
        event.coord + ChunkPos(WDIR_NORTH),
        event.coord + ChunkPos(WDIR_SOUTH),
        event.coord + ChunkPos(WDIR_EAST),
        event.coord + ChunkPos(WDIR_WEST),
        event.coord + ChunkPos(WDIR_UP),
        event.coord + ChunkPos(WDIR_DOWN),
    };

    globals::registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    for(const ChunkPos &cpos : neighbours) {
        if(const Chunk *chunk = world::find_chunk(cpos)) {
            globals::registry.emplace_or_replace<NeedsMeshingComponent>(chunk->entity);
            continue;
        }
    }
}

static void on_chunk_remove(const ChunkRemoveEvent &event)
{
    const auto it = workers.find(event.coord);
    if(it == workers.cend())
        return;
    it->second->is_cancelled = true;
}

static void on_voxel_set(const VoxelSetEvent &event)
{
    globals::registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    std::vector<ChunkPos> neighbours = {};

    for(int dim = 0; dim < 3; ++dim) {
        ChunkPos offset = ChunkPos(0, 0, 0);
        offset[dim] = 1;

        if(event.lpos[dim] == 0) {
            neighbours.push_back(event.cpos - offset);
            continue;
        }

        if(event.lpos[dim] == (CHUNK_SIZE - 1)) {
            neighbours.push_back(event.cpos + offset);
            continue;
        }
    }

    for(const ChunkPos &cpos : neighbours) {
        if(const Chunk *chunk = world::find_chunk(cpos)) {
            globals::registry.emplace_or_replace<NeedsMeshingComponent>(chunk->entity);
            continue;
        }
    }
}

void chunk_mesher::init(void)
{
    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<VoxelSetEvent>().connect<&on_voxel_set>();
}

void chunk_mesher::deinit(void)
{
    for(auto &worker : workers)
        worker.second->is_cancelled = true;
    workers_pool.wait_for_tasks();
    workers.clear();
}

void chunk_mesher::update(void)
{
    std::size_t finalized = 0;
    std::size_t enqueued = 0;

    auto worker = workers.cbegin();
    while(worker != workers.cend()) {
        if(!worker->second->future.valid()) {
            ++worker;
            continue;
        }

        if(worker->second->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            ++worker;
            continue;
        }

        if(worker->second->is_cancelled) {
            worker = workers.erase(worker);
            continue;
        }

        if(const Chunk *chunk = world::find_chunk(worker->second->coord)) {
            if(globals::registry.any_of<NeedsMeshingComponent>(chunk->entity)) {
                worker = workers.erase(worker);
                continue;
            }

            finalize(worker->second.get(), chunk->entity);
            finalized += 1U;
        }

        worker = workers.erase(worker);

        if(finalized >= MESHER_TASKS_PER_FRAME)
            break;
        continue;
    }

    const auto group = globals::registry.group<NeedsMeshingComponent>(entt::get<ChunkComponent>);
    for(const auto [entity, chunk] : group.each()) {
        const auto it = workers.find(chunk.coord);

        if(it == workers.cend()) {
            globals::registry.remove<NeedsMeshingComponent>(entity);

            auto &worker = workers.emplace(chunk.coord, std::make_unique<WorkerContext>()).first->second;
            worker->coord = chunk.coord;

            cache_chunk(worker.get(), chunk.coord);
            cache_chunk(worker.get(), chunk.coord + ChunkPos(WDIR_NORTH));
            cache_chunk(worker.get(), chunk.coord + ChunkPos(WDIR_SOUTH));
            cache_chunk(worker.get(), chunk.coord + ChunkPos(WDIR_EAST));
            cache_chunk(worker.get(), chunk.coord + ChunkPos(WDIR_WEST));
            cache_chunk(worker.get(), chunk.coord + ChunkPos(WDIR_UP));
            cache_chunk(worker.get(), chunk.coord + ChunkPos(WDIR_DOWN));

            worker->future = workers_pool.submit(std::bind(&process, worker.get()));

            enqueued += 1U;
        }
        else {
            it->second->is_cancelled = true;
            continue;
        }

        if(enqueued >= MESHER_TASKS_PER_FRAME)
            break;
        continue;
    }

    if(finalized || enqueued) {
        spdlog::info("mesher: {} {}", finalized, enqueued);
    }
}
