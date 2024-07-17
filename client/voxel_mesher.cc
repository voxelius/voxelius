// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/entity/voxel_mesh.hh>
#include <client/globals.hh>
#include <client/voxel_atlas.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_vertex.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/entity/chunk.hh>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_fill.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/voxel_set.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>
#include <thread_pool.hpp>

using VoxelMeshBuilder = MeshBuilder<VoxelVertex>;

using chunk_cache_id_t = unsigned short;
constexpr static const chunk_cache_id_t CHUNK_CACHE_ITSELF  = 0x0000;
constexpr static const chunk_cache_id_t CHUNK_CACHE_NORTH   = 0x0001;
constexpr static const chunk_cache_id_t CHUNK_CACHE_SOUTH   = 0x0002;
constexpr static const chunk_cache_id_t CHUNK_CACHE_EAST    = 0x0003;
constexpr static const chunk_cache_id_t CHUNK_CACHE_WEST    = 0x0004;
constexpr static const chunk_cache_id_t CHUNK_CACHE_TOP     = 0x0005;
constexpr static const chunk_cache_id_t CHUNK_CACHE_BOTTOM  = 0x0006;
constexpr static const size_t NUM_CHUNK_CACHE = 7;

// OPTIMIZATION:
// Instead of hashing chunk positions every time,
// we just have a simple vector->index function and
// keep cached chunks in a linear array.
static const chunk_cache_id_t get_chunk_cache_id(const ChunkCoord &pivot, const ChunkCoord &cv)
{
    static const chunk_cache_id_t nx[3] = {CHUNK_CACHE_WEST, 0, CHUNK_CACHE_EAST};
    static const chunk_cache_id_t ny[3] = {CHUNK_CACHE_BOTTOM, 0, CHUNK_CACHE_TOP};
    static const chunk_cache_id_t nz[3] = {CHUNK_CACHE_NORTH, 0, CHUNK_CACHE_SOUTH};

    if(pivot != cv) {
        ChunkCoord delta = (pivot - cv);
        delta.x = cxmath::clamp(delta.x, -1, 1);
        delta.y = cxmath::clamp(delta.y, -1, 1);
        delta.z = cxmath::clamp(delta.z, -1, 1);

        if(delta.x)
            return nx[delta.x + 1];
        if(delta.y)
            return ny[delta.y + 1];
        return nz[delta.z + 1];
    }

    return CHUNK_CACHE_ITSELF;
}

static const LocalCoord get_face_direction(VoxelFace face)
{
    switch(face) {
        case VOXEL_FACE_NORTH:
            return LocalCoord{0, 0, -1};
        case VOXEL_FACE_SOUTH:
            return LocalCoord{0, 0, 1};
        case VOXEL_FACE_EAST:
            return LocalCoord{1, 0, 0};
        case VOXEL_FACE_WEST:
            return LocalCoord{-1, 0, 0};
        case VOXEL_FACE_TOP:
            return LocalCoord{0, 1, 0};
        case VOXEL_FACE_BOTTOM:
            return LocalCoord{0, -1, 0};
        default:
            return LocalCoord{0, 0, 0};
    }
}

class VoxelMeshWorker final : public NonCopyable {
public:
    // OPTIMIZATION:
    // Instead of using any thread locking mechanisms to
    // avoid race conditions we just have cached voxel storage
    // so threaded workers are nicely isolated frome each other.
    void preserve_chunk(const ChunkCoord &cv);

    // Obstructive voxel is one of the following:
    //  1. Voxel of the same kin (Voxel equation)
    //  2. Voxel of the different draw mode.
    // Returns FALSE when there is an obstructive voxel at lv
    bool test(Voxel voxel, const VoxelInfo *info, const LocalCoord &lv) const;

    void prepare();
    void process();
    void finalize(entt::entity entity);

public:
    ChunkCoord coord {};
    bool cancelled {false};
    std::array<VoxelStorage, NUM_CHUNK_CACHE> cache {};
    std::array<VoxelMeshBuilder, NUM_VOXEL_DRAW> builders {};
    std::shared_future<bool> future {};
};

void VoxelMeshWorker::preserve_chunk(const ChunkCoord &cv)
{
    if(const Chunk *chunk = world::find_chunk(cv)) {
        const auto idx = get_chunk_cache_id(this->coord, cv);
        voxel_storage::clone(cache[idx], chunk->storage);
    }
}

bool VoxelMeshWorker::test(Voxel voxel, const VoxelInfo *info, const LocalCoord &lv) const
{
    const auto pvv = coord::to_voxel(coord, lv);
    const auto pcv = coord::to_chunk(pvv);
    const auto plv = coord::to_local(pvv);
    const auto idx = coord::to_index(plv);

    const auto cache_id = get_chunk_cache_id(coord, pcv);
    const auto &storage = cache.at(cache_id);
    const auto neighbour = voxel_storage::get(storage, idx);

    if(neighbour == NULL_VOXEL) {
        // Air is transparent so the
        // face we're checking for is visible
        return true;
    }

    if(neighbour == voxel) {
        // It's of our kin, the face
        // is not henceforth visible
        return false;
    }

    if(const VoxelInfo *neighbour_info = vdef::find(neighbour)) {
        if(neighbour_info->draw != info->draw) {
            // The neighbouring voxel is drawn (or
            // not if it's NODRAW) using a different
            // shader and a different mesh, the face is visible
            return true;
        }
    }

    return false;
}

void VoxelMeshWorker::prepare()
{
    preserve_chunk(coord);
    preserve_chunk(coord + ChunkCoord{0, 0, 1});
    preserve_chunk(coord - ChunkCoord{0, 0, 1});
    preserve_chunk(coord + ChunkCoord{0, 1, 0});
    preserve_chunk(coord - ChunkCoord{0, 1, 0});
    preserve_chunk(coord + ChunkCoord{1, 0, 0});
    preserve_chunk(coord - ChunkCoord{1, 0, 0});
}

void VoxelMeshWorker::process()
{
    const VoxelStorage &storage = cache.at(CHUNK_CACHE_ITSELF);

    // OPTIMIZATION:
    // Instead of iterating through every single
    // voxel type possible, it's a good idea to
    // just keep track of voxels present within the chunk
    std::unordered_set<Voxel> presence = {};

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto voxel = voxel_storage::get(storage, i);
        if(voxel == NULL_VOXEL)
            continue;
        presence.emplace(voxel);
    }

    for(const auto voxel : presence) {
        if(const VoxelInfo *info = vdef::find(voxel)) {
            if(info->draw == VOXEL_DRAW_NODRAW) {
                // There isn't a mesh builder
                // for invisible voxels!
                continue;
            }

            for(VoxelFace face = 0; face < NUM_VOXEL_FACE; ++face) {
                std::array<bool, CHUNK_AREA> mask = {};

                const VoxelTexture &vtex = info->textures[face];

                if(vtex.paths.empty()) {
                    // Something went TERRIBLY WRONG
                    // either during altas setup or during
                    // voxel_anims::construct()
                    continue;
                }

                unsigned int d = 0U;
                switch(face) {
                    case VOXEL_FACE_EAST:
                    case VOXEL_FACE_WEST:
                        d = 0U; // X axis
                        break;
                    case VOXEL_FACE_TOP:
                    case VOXEL_FACE_BOTTOM:
                        d = 1U; // Y axis
                        break;
                    case VOXEL_FACE_NORTH:
                    case VOXEL_FACE_SOUTH:
                        d = 2U; // Z axis
                        break;
                    default:
                        // Non-cubic face
                        continue;
                }

                const unsigned int u = (d + 1U) % 3U;
                const unsigned int v = (d + 2U) % 3U;
                const LocalCoord q = get_face_direction(face);

                LocalCoord x = {0, 0, 0};

                for(x[d] = 0; x[d] < CHUNK_SIZE;) {
                    size_t mpos;

                    mask.fill(false);

                    mpos = 0;
                    for(x[v] = 0; x[v] < CHUNK_SIZE; ++x[v]) {
                        for(x[u] = 0; x[u] < CHUNK_SIZE; ++x[u]) {
                            if(voxel_storage::get(storage, x) == voxel)
                                mask[mpos] = test(voxel, info, x + q);
                            ++mpos;
                        }
                    }

                    ++x[d];

                    mpos = 0;
                    for(unsigned int j = 0U; j < CHUNK_SIZE; ++j) {
                        for(unsigned int i = 0U; i < CHUNK_SIZE;) {
                            if(mask[mpos]) {
                                unsigned int qw;
                                unsigned int qh;

                                // Quad width
                                for(qw = 1U; (i + qw) < CHUNK_SIZE && mask[mpos + qw]; ++qw);

                                // Quad height
                                bool qh_complete = false;
                                for(qh = 1U; (j + qh) < CHUNK_SIZE; ++qh) {
                                    for(unsigned int k = 0U; k < qw; ++k) {
                                        if(!mask[mpos + k + qh * CHUNK_SIZE]) {
                                            qh_complete = true;
                                            break;
                                        }
                                    }

                                    if(qh_complete) {
                                        // We are done
                                        break;
                                    }
                                }

                                x[u] = i;
                                x[v] = j;

                                glm::dvec3 pos = glm::dvec3{x};

                                if(q[d] < 0) {
                                    // Since we increased x[d] before, faces
                                    // with positive normals are in their place
                                    // but faces with negative normals need to
                                    // be nudged in the normal's direction.
                                    pos[d] += static_cast<double>(q[d]);
                                }

                                glm::dvec2 uvs[4] = {};
                                glm::dvec2 tc = glm::dvec2{qw, qh};
                                unsigned int shade = 0U;

                                switch(face) {
                                    case VOXEL_FACE_WEST:
                                        uvs[0] = glm::dvec2{0.0, 0.0};
                                        uvs[1] = glm::dvec2{tc.y, 0.0};
                                        uvs[2] = glm::dvec2{tc.y, tc.x};
                                        uvs[3] = glm::dvec2{0.0, tc.x};
                                        shade = 1U; // 0.6
                                        break;
                                    case VOXEL_FACE_EAST:
                                        uvs[0] = glm::dvec2{tc.y, 0.0};
                                        uvs[1] = glm::dvec2{tc.y, tc.x};
                                        uvs[2] = glm::dvec2{0.0, tc.x};
                                        uvs[3] = glm::dvec2{0.0, 0.0};
                                        shade = 1U; // 0.6
                                        break;
                                    case VOXEL_FACE_SOUTH:
                                        uvs[0] = glm::dvec2{0.0, 0.0};
                                        uvs[1] = glm::dvec2{tc.x, 0.0};
                                        uvs[2] = glm::dvec2{tc.x, tc.y};
                                        uvs[3] = glm::dvec2{0.0, tc.y};
                                        shade = 2U; // 0.8
                                        break;
                                    case VOXEL_FACE_NORTH:
                                        uvs[0] = glm::dvec2{0.0, 0.0};
                                        uvs[1] = glm::dvec2{0.0, tc.y};
                                        uvs[2] = glm::dvec2{tc.x, tc.y};
                                        uvs[3] = glm::dvec2{tc.x, 0.0};
                                        shade = 2U; // 0.8
                                        break;
                                    case VOXEL_FACE_TOP:
                                        uvs[0] = glm::dvec2{0.0, tc.x};
                                        uvs[1] = glm::dvec2{0.0, 0.0};
                                        uvs[2] = glm::dvec2{tc.y, 0.0};
                                        uvs[3] = glm::dvec2{tc.y, tc.x};
                                        shade = 3U; // 1.0
                                        break;
                                    case VOXEL_FACE_BOTTOM:
                                        uvs[0] = glm::dvec2{tc.y, tc.x};
                                        uvs[1] = glm::dvec2{0.0, tc.x};
                                        uvs[2] = glm::dvec2{0.0, 0.0};
                                        uvs[3] = glm::dvec2{tc.y, 0.0};
                                        shade = 0U; // 0.4
                                        break;
                                }

                                glm::dvec3 du = {0.0, 0.0, 0.0};
                                du[u] = static_cast<double>(qw);

                                glm::dvec3 dv = {0.0, 0.0, 0.0};
                                dv[v] = static_cast<double>(qh);

                                VoxelVertex verts[4] = {};
                                const uint16_t toffset = vtex.cached_offset;
                                const uint16_t tframes = vtex.paths.size();

                                if(q[d] < 0) {
                                    verts[0] = VoxelVertex{pos,             shade, toffset, tframes, uvs[0]};
                                    verts[1] = VoxelVertex{pos + dv,        shade, toffset, tframes, uvs[1]};
                                    verts[2] = VoxelVertex{pos + du + dv,   shade, toffset, tframes, uvs[2]};
                                    verts[3] = VoxelVertex{pos + du,        shade, toffset, tframes, uvs[3]};
                                }
                                else {
                                    verts[0] = VoxelVertex{pos,             shade, toffset, tframes, uvs[0]};
                                    verts[1] = VoxelVertex{pos + du,        shade, toffset, tframes, uvs[1]};
                                    verts[2] = VoxelVertex{pos + dv + du,   shade, toffset, tframes, uvs[2]};
                                    verts[3] = VoxelVertex{pos + dv,        shade, toffset, tframes, uvs[3]};
                                }

                                VoxelMeshBuilder &builder = builders[info->draw];
                                builder.push_back(verts[0], verts[1], verts[2], verts[3]);

                                for(unsigned int k = 0U; k < qh; ++k) {
                                    for(unsigned int l = 0U; l < qw; ++l) {
                                        mask[mpos + l + k * CHUNK_SIZE] = false;
                                    }
                                }

                                i += qw;
                                mpos += qw;
                            }
                            else {
                                ++i;
                                ++mpos;
                            }
                        }
                    }
                }
            }
        }
    }
}

void VoxelMeshWorker::finalize(entt::entity entity)
{
    auto &mc = globals::registry.get_or_emplace<VoxelMeshComponent>(entity);

    for(VoxelDraw draw = 0; draw < NUM_VOXEL_DRAW; ++draw) {
        Mesh &mref = mc.meshes.at(draw);

        const auto &builder = builders.at(draw);

        if(builder.empty()) {
            mref.vertices = 0;
            mref.triangles = 0;
            mref.vbo.destroy();
            continue;
        }

        if(!mref.vbo.get())
            mref.vbo.create();
        mref.vbo.resize(builder.bytes(), builder.data(), GL_STATIC_DRAW);
        mref.vertices = builder.size();
        mref.triangles = 0; // FIXME
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
constexpr static const size_t MESHER_THREADS_COUNT = 4;
constexpr static const size_t MESHER_TASKS_PER_FRAME = 32;
#endif

static thread_pool workers_pool = thread_pool{MESHER_THREADS_COUNT};
static std::unordered_map<ChunkCoord, std::unique_ptr<VoxelMeshWorker>> workers = {};

// Bogus internal flag component
struct NeedsMeshingComponent final {};

static void on_chunk_create(const ChunkCreateEvent &event)
{
    const std::array<ChunkCoord, 6> neighbours = {
        event.coord + ChunkCoord{0, 0, 1},
        event.coord - ChunkCoord{0, 0, 1},
        event.coord + ChunkCoord{0, 1, 0},
        event.coord - ChunkCoord{0, 1, 0},
        event.coord + ChunkCoord{1, 0, 0},
        event.coord - ChunkCoord{1, 0, 0},
    };

    globals::registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    for(const auto ncv : neighbours) {
        if(const Chunk *nchunk = world::find_chunk(ncv)) {
            globals::registry.emplace_or_replace<NeedsMeshingComponent>(nchunk->entity);
        }
    }
}

static void on_chunk_remove(const ChunkRemoveEvent &event)
{
    const auto it = workers.find(event.coord);
    if(it == workers.cend())
        return;
    it->second->cancelled = true;
}

static void on_voxel_set(const VoxelSetEvent &event)
{
    globals::registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    // OPTIMIZATION:
    // Old code used to queue ALL existing neighbours
    // for meshing which introduced a set of performance
    // issues. Now we only queue neighbours if it's strictly
    // necessary. In other words, putting a voxel right
    // smack in the middle of a chunk is as fast as it can get.
    std::unordered_set<ChunkCoord> neighbours = {};

    for(int dim = 0; dim < 3; ++dim) {
        ChunkCoord offset = ChunkCoord{0, 0, 0};
        offset[dim] = 1;

        if(event.lv[dim] == 0) {
            neighbours.emplace(event.cv - offset);
            continue;
        }

        if(event.lv[dim] == CHUNK_SIZE - 1) {
            neighbours.emplace(event.cv + offset);
            continue;
        }
    }

    for(const ChunkCoord &ncv : neighbours) {
        if(const Chunk *nchunk = world::find_chunk(ncv)) {
            globals::registry.emplace_or_replace<NeedsMeshingComponent>(nchunk->entity);
        }
    }
}

void voxel_mesher::init()
{
    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<VoxelSetEvent>().connect<&on_voxel_set>();
}

void voxel_mesher::deinit()
{
    for(auto &it : workers)
        it.second->cancelled = true;
    workers_pool.wait_for_tasks();
    workers.clear();
}

void voxel_mesher::update()
{
    size_t num_queued = 0;
    size_t num_finalized = 0;

    for(auto worker = workers.begin(); worker != workers.end();) {
        if(worker->second->future.valid()) {
            if(worker->second->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                if(const Chunk *chunk = world::find_chunk(worker->second->coord)) {
                    worker->second->finalize(chunk->entity);
                    ++num_finalized;
                }

                worker = workers.erase(worker);

                if(num_finalized >= MESHER_TASKS_PER_FRAME)
                    break;
                continue;
            }
        }

        ++worker;
    }

    const auto group = globals::registry.group<NeedsMeshingComponent>(entt::get<ChunkComponent>);

    for(const auto [entity, chunk] : group.each()) {
        if(workers.find(chunk.coord) == workers.cend()) {
            globals::registry.remove<NeedsMeshingComponent>(entity);

            auto &worker = workers.emplace(chunk.coord, std::make_unique<VoxelMeshWorker>()).first->second;
            worker->coord = chunk.coord;
            worker->prepare();

            // FIXME: have a Sodium-like setting to force nearby chunks
            // to be meshed in the main thread to avoid visual glitches
            worker->future = workers_pool.submit(std::bind(&VoxelMeshWorker::process, worker.get()));

            ++num_queued;
        }

        if(num_queued >= MESHER_TASKS_PER_FRAME)
            break;
        continue;
    }

    if(num_finalized || num_queued) {
        // Output only when we have something to output
        spdlog::debug("voxel_mesher: {} {}", num_finalized, num_queued);
    }
}
