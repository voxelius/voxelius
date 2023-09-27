// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/globals.hh>
#include <client/voxel_mesher.hh>
#include <shared/cxmath.hh>
#include <shared/world.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>
#include <thread_pool.hpp>
#include <unordered_set>

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
static const chunk_cache_id_t get_chunk_cache_id(const chunk_pos_t &pivot, const chunk_pos_t &cpos)
{
    static const chunk_cache_id_t nx[3] = {CHUNK_CACHE_WEST, 0, CHUNK_CACHE_EAST};
    static const chunk_cache_id_t ny[3] = {CHUNK_CACHE_BOTTOM, 0, CHUNK_CACHE_TOP};
    static const chunk_cache_id_t nz[3] = {CHUNK_CACHE_NORTH, 0, CHUNK_CACHE_SOUTH};

    if(pivot != cpos) {
        chunk_pos_t delta = (pivot - cpos);
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

static const local_pos_t get_face_direction(voxel_face_t face)
{
    switch(face) {
        case VOXEL_FACE_NORTH:
            return local_pos_t{0, 0, -1};
        case VOXEL_FACE_SOUTH:
            return local_pos_t{0, 0, 1};
        case VOXEL_FACE_EAST:
            return local_pos_t{1, 0, 0};
        case VOXEL_FACE_WEST:
            return local_pos_t{-1, 0, 0};
        case VOXEL_FACE_TOP:
            return local_pos_t{0, 1, 0};
        case VOXEL_FACE_BOTTOM:
            return local_pos_t{0, -1, 0};
        default:
            return local_pos_t{0, 0, 0};
    }
}

class VMeshWorker final : public mixin::NonCopyable {
public:
    // OPTIMIZATION:
    // Instead of using any thread locking mechanisms to
    // avoid race conditions we just have a cached voxels
    // so threaded workers are nicely isolated frome each other.
    void preserve_chunk(const chunk_pos_t &cpos);

    // Obstructive voxel is one of the following:
    //  1. Voxel of the same kin (voxel_t equation)
    //  2. Voxel of the different draw mode.
    // Returns FALSE when there is an obstructive voxel at nlpos
    bool test(voxel_t voxel, const VoxelInfo *info, const local_pos_t &nlpos) const;

    void prepare();
    void process();
    void finalize(entt::entity entity);

public:
    chunk_pos_t cpos {};
    bool cancelled {false};
    std::array<voxel_array_t, NUM_CHUNK_CACHE> chunks {};
    std::array<VoxelMeshBuilder, NUM_VOXEL_DRAW> builders {};
    std::shared_future<bool> future {};
};

void VMeshWorker::preserve_chunk(const chunk_pos_t &cpos)
{
    if(const Chunk *chunk = globals::world.find_chunk(cpos)) {
        const auto idx = get_chunk_cache_id(this->cpos, cpos);
        chunks[idx] = chunk->voxels;
    }
}

bool VMeshWorker::test(voxel_t voxel, const VoxelInfo *info, const local_pos_t &nlpos) const
{
    const auto patch_vpos = coord::to_voxel(cpos, nlpos);
    const auto patch_cpos = coord::to_chunk(patch_vpos);
    const auto patch_lpos = coord::to_local(patch_vpos);
    const auto patch_index = coord::to_index(patch_lpos);

    const auto idx = get_chunk_cache_id(cpos, patch_cpos);
    const auto &chunk = chunks.at(idx);
    const auto nvoxel = chunk.at(patch_index);

    if(nvoxel == NULL_VOXEL) {
        // Air is transparent, the face
        // is in fact visible.
        return true;
    }

    if(nvoxel == voxel) {
        // Voxel's clone is opaque, the
        // face is not visible.
        return false;
    }

    const auto nid = get_voxel_id(nvoxel);
    const auto nstate = get_voxel_state(nvoxel);

    if(nid == NULL_VOXEL_ID) {
        // What the fuck? Air with a
        // different state? Does it smell
        // different or something?
        return true;
    }

    if(const VoxelInfo *ninfo = vdef::find(nid, nstate)) {
        if(ninfo->draw != info->draw) {
            // The neighbouring voxel is drawn (or
            // not if it's NODRAW) using a different
            // shader and a different mesh, the face is visible
            return true;
        }
    }

    return false;
}

void VMeshWorker::prepare()
{
    preserve_chunk(cpos);
    preserve_chunk(cpos + chunk_pos_t{0, 0, 1});
    preserve_chunk(cpos - chunk_pos_t{0, 0, 1});
    preserve_chunk(cpos + chunk_pos_t{0, 1, 0});
    preserve_chunk(cpos - chunk_pos_t{0, 1, 0});
    preserve_chunk(cpos + chunk_pos_t{1, 0, 0});
    preserve_chunk(cpos - chunk_pos_t{1, 0, 0});
}

void VMeshWorker::process()
{
    const voxel_array_t &chunk = chunks[CHUNK_CACHE_ITSELF];

    // OPTIMIZATION:
    // Instead of iterating through every single
    // voxel type possible, it's a good idea to
    // just keep track of voxels present within the chunk
    std::unordered_set<voxel_t> presence = {};

    for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto voxel = chunk.at(i);
        presence.emplace(voxel);
    }

    for(const auto voxel : presence) {
        if(const VoxelInfo *info = vdef::find(voxel)) {
            if(info->draw == VOXEL_DRAW_NODRAW) {
                // There isn't a mesh builder
                // for invisible voxels!
                continue;
            }

            for(voxel_face_t face = 0; face < NUM_VOXEL_FACE; ++face) {
                std::array<bool, CHUNK_AREA> mask = {};

                const AtlasTexture *atex = atlas::find(info->textures[face].cache);

                if(atex == nullptr) {
                    // FIX 2023-09-26:
                    // The vdef cache is now set up right after
                    // we fill it up since we now can iterate through
                    // all the textures found in the table.
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
                const local_pos_t q = get_face_direction(face);
                const vec3f_t norm = vec3f_t{q.x, q.y, q.z};

                local_pos_t x = {0, 0, 0};

                for(x[d] = 0; x[d] < CHUNK_SIZE;) {
                    size_t mpos;

                    mask.fill(false);

                    mpos = 0;
                    for(x[v] = 0; x[v] < CHUNK_SIZE; ++x[v]) {
                        for(x[u] = 0; x[u] < CHUNK_SIZE; ++x[u]) {
                            if(chunk.at(coord::to_index(x)) == voxel)
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
                                for(qw = 1U; (mpos + qw) < CHUNK_SIZE && mask[mpos + qw]; ++qw);

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

                                vec3f_t pos = vec3f_t{x};

                                if(q[d] < 0) {
                                    // Since we increased x[d] before, faces
                                    // with positive normals are in their place
                                    // but faces with negative normals need to
                                    // be nudged in the normal's direction.
                                    pos[d] += static_cast<float>(q[d]);
                                }

                                vec2f_t uvs[4] = {};
                                vec2f_t tc = vec2f_t{qw, qh} * atex->max_uv;

                                switch(face) {
                                    case VOXEL_FACE_WEST:
                                        uvs[0] = vec2f_t{0.0f, 0.0f};
                                        uvs[1] = vec2f_t{tc.y, 0.0f};
                                        uvs[2] = vec2f_t{tc.y, tc.x};
                                        uvs[3] = vec2f_t{0.0f, tc.x};
                                        break;
                                    case VOXEL_FACE_EAST:
                                        uvs[0] = vec2f_t{tc.y, 0.0f};
                                        uvs[1] = vec2f_t{tc.y, tc.x};
                                        uvs[2] = vec2f_t{0.0f, tc.x};
                                        uvs[3] = vec2f_t{0.0f, 0.0f};
                                        break;
                                    case VOXEL_FACE_SOUTH:
                                        uvs[0] = vec2f_t{0.0f, 0.0f};
                                        uvs[1] = vec2f_t{tc.x, 0.0f};
                                        uvs[2] = vec2f_t{tc.x, tc.y};
                                        uvs[3] = vec2f_t{0.0f, tc.y};
                                        break;
                                    case VOXEL_FACE_NORTH:
                                        uvs[0] = vec2f_t{0.0f, 0.0f};
                                        uvs[1] = vec2f_t{0.0f, tc.y};
                                        uvs[2] = vec2f_t{tc.x, tc.y};
                                        uvs[3] = vec2f_t{tc.x, 0.0f};
                                        break;
                                    case VOXEL_FACE_TOP:
                                        uvs[0] = vec2f_t{0.0f, tc.x};
                                        uvs[1] = vec2f_t{0.0f, 0.0f};
                                        uvs[2] = vec2f_t{tc.y, 0.0f};
                                        uvs[3] = vec2f_t{tc.y, tc.x};
                                        break;
                                    case VOXEL_FACE_BOTTOM:
                                        uvs[0] = vec2f_t{tc.y, tc.x};
                                        uvs[1] = vec2f_t{0.0f, tc.x};
                                        uvs[2] = vec2f_t{0.0f, 0.0f};
                                        uvs[3] = vec2f_t{tc.y, 0.0f};
                                        break;
                                }

                                vec3f_t du = {0.0f, 0.0f, 0.0f};
                                du[u] = static_cast<float>(qw);

                                vec3f_t dv = {0.0f, 0.0f, 0.0f};
                                dv[v] = static_cast<float>(qh);

                                VoxelVertex verts[4] = {};

                                if(q[d] < 0) {
                                    verts[0] = VoxelVertex{pos, norm, uvs[0], atex->tex_id};
                                    verts[1] = VoxelVertex{pos + dv, norm, uvs[1], atex->tex_id};
                                    verts[2] = VoxelVertex{pos + du + dv, norm, uvs[2], atex->tex_id};
                                    verts[3] = VoxelVertex{pos + du, norm, uvs[3], atex->tex_id};
                                }
                                else {
                                    verts[0] = VoxelVertex{pos, norm, uvs[0], atex->tex_id};
                                    verts[1] = VoxelVertex{pos + du, norm, uvs[1], atex->tex_id};
                                    verts[2] = VoxelVertex{pos + dv + du, norm, uvs[2], atex->tex_id};
                                    verts[3] = VoxelVertex{pos + dv, norm, uvs[3], atex->tex_id};
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

void VMeshWorker::finalize(entt::entity entity)
{
    auto &mc = globals::world.registry.get_or_emplace<VoxelMeshComponent>(entity);

    for(voxel_draw_t draw = 0; draw < NUM_VOXEL_DRAW; ++draw) {
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
constexpr static const size_t MESHER_THREADS_COUNT = 2;
constexpr static const size_t MESHER_TASKS_PER_FRAME = 32;
#endif

static thread_pool workers_pool = thread_pool{MESHER_THREADS_COUNT};
static std::unordered_map<chunk_pos_t, std::unique_ptr<VMeshWorker>> workers = {};

// Bogus internal flag component
struct NeedsMeshingComponent final {};

static void on_chunk_create(const ChunkCreateEvent &event)
{
    const std::array<chunk_pos_t, 6> neighbours = {
        event.cpos + chunk_pos_t{0, 0, 1},
        event.cpos - chunk_pos_t{0, 0, 1},
        event.cpos + chunk_pos_t{0, 1, 0},
        event.cpos - chunk_pos_t{0, 1, 0},
        event.cpos + chunk_pos_t{1, 0, 0},
        event.cpos - chunk_pos_t{1, 0, 0},
    };

    event.world->registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    for(const auto ncpos : neighbours) {
        if(const Chunk *nchunk = event.world->find_chunk(ncpos)) {
            event.world->registry.emplace_or_replace<NeedsMeshingComponent>(nchunk->entity);
        }
    }
}

static void on_chunk_remove(const ChunkRemoveEvent &event)
{
    const auto it = workers.find(event.cpos);
    if(it == workers.cend())
        return;
    it->second->cancelled = true;
}

static void on_voxel_update(const VoxelUpdateEvent &event)
{
    event.world->registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    // OPTIMIZATION:
    // Old code used to queue ALL existing neighbours
    // for meshing which introduced a set of performance
    // issues. Now we only queue neighbours if it's strictly
    // necessary. In other words, putting a voxel right
    // smack in the middle of a chunk is as fast as it can get.
    std::unordered_set<chunk_pos_t> neighbours = {};

    for(int dim = 0; dim < 3; ++dim) {
        chunk_pos_t offset = chunk_pos_t{0, 0, 0};
        offset[dim] = 1;

        if(event.lpos[dim] == 0) {
            neighbours.emplace(event.cpos - offset);
            continue;
        }

        if(event.lpos[dim] == CHUNK_SIZE - 1) {
            neighbours.emplace(event.cpos + offset);
            continue;
        }
    }

    for(const chunk_pos_t &ncpos : neighbours) {
        if(const Chunk *nchunk = event.world->find_chunk(ncpos)) {
            event.world->registry.emplace_or_replace<NeedsMeshingComponent>(nchunk->entity);
        }
    }
}

void voxel_mesher::init()
{
    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<VoxelUpdateEvent>().connect<&on_voxel_update>();
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
                if(const Chunk *chunk = globals::world.find_chunk(worker->second->cpos)) {
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

    const auto group = globals::world.registry.group<NeedsMeshingComponent>(entt::get<ChunkComponent>);

    for(const auto [entity, chunk] : group.each()) {
        if(workers.find(chunk.cpos) == workers.cend()) {
            globals::world.registry.remove<NeedsMeshingComponent>(entity);

            auto &worker = workers.emplace(chunk.cpos, std::make_unique<VMeshWorker>()).first->second;
            worker->cpos = chunk.cpos;
            worker->prepare();

            // FIXME: have a Sodium-like setting to force nearby chunks
            // to be meshed in the main thread to avoid visual glitches
            worker->future = workers_pool.submit(std::bind(&VMeshWorker::process, worker.get()));

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
