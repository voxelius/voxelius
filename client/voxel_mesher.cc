// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/globals.hh>
#include <client/vertex.hh>
#include <client/voxel_mesher.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>
#include <thread_pool.hpp>
#include <unordered_set>

struct NeedsMeshingComponent final {};

// FIXME: forward-port packed VoxelVertex
using VoxelMeshBuilder = MeshBuilder<Vertex>;

struct VoxelFaceQuad final {
    local_coord_t offs[4] {};
    vec2f_t texcoords[4] {};
    vec3f_t norm {};
};

using chunk_cache_id_t = unsigned int;
constexpr static const chunk_cache_id_t CHUNK_CACHE_ITSELF  = 0;
constexpr static const chunk_cache_id_t CHUNK_CACHE_NORTH   = 1;
constexpr static const chunk_cache_id_t CHUNK_CACHE_SOUTH   = 2;
constexpr static const chunk_cache_id_t CHUNK_CACHE_EAST    = 3;
constexpr static const chunk_cache_id_t CHUNK_CACHE_WEST    = 4;
constexpr static const chunk_cache_id_t CHUNK_CACHE_TOP     = 5;
constexpr static const chunk_cache_id_t CHUNK_CACHE_BOTTOM  = 6;
constexpr static const size_t NUM_CHUNK_CACHE = 7;

static bool is_cube_face(voxel_face_t face)
{
    switch(face) {
        case VOXEL_FACE_NORTH:
        case VOXEL_FACE_SOUTH:
        case VOXEL_FACE_EAST:
        case VOXEL_FACE_WEST:
        case VOXEL_FACE_TOP:
        case VOXEL_FACE_BOTTOM:
            return true;
        default:
            return false;
    }
}

// OPTIMIZATION:
// Instead of hashing chunk positions every time,
// we just have a simple vector->index function and
// keep cached chunks in a linear array.
static const chunk_cache_id_t get_chunk_cache_id(const chunk_coord_t &pivot, const chunk_coord_t &cpos)
{
    static const chunk_cache_id_t nx[3] = {CHUNK_CACHE_WEST, 0, CHUNK_CACHE_EAST};
    static const chunk_cache_id_t ny[3] = {CHUNK_CACHE_BOTTOM, 0, CHUNK_CACHE_TOP};
    static const chunk_cache_id_t nz[3] = {CHUNK_CACHE_NORTH, 0, CHUNK_CACHE_SOUTH};

    if(pivot != cpos) {
        chunk_coord_t delta = (pivot - cpos);
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

static const local_coord_t get_face_direction(voxel_face_t face)
{
    switch(face) {
        case VOXEL_FACE_NORTH:
            return local_coord_t{0, 0, -1};
        case VOXEL_FACE_SOUTH:
            return local_coord_t{0, 0, 1};
        case VOXEL_FACE_EAST:
            return local_coord_t{1, 0, 0};
        case VOXEL_FACE_WEST:
            return local_coord_t{-1, 0, 0};
        case VOXEL_FACE_TOP:
            return local_coord_t{0, 1, 0};
        case VOXEL_FACE_BOTTOM:
            return local_coord_t{0, -1, 0};
        default:
            return local_coord_t{0, 0, 0};
    }
}

static bool get_face_quad(VoxelFaceQuad &out, voxel_face_t face, voxel_shape_t shape)
{
    if(shape == VOXEL_SHAPE_CUBE) {
        switch(face) {
            case VOXEL_FACE_NORTH:
                out.offs[0] = local_coord_t{0, 0, 0};
                out.offs[1] = local_coord_t{0, 1, 0};
                out.offs[2] = local_coord_t{1, 1, 0};
                out.offs[3] = local_coord_t{1, 0, 0};
                out.texcoords[0] = vec2f_t{1.0f, 0.0f};
                out.texcoords[1] = vec2f_t{1.0f, 1.0f};
                out.texcoords[2] = vec2f_t{0.0f, 1.0f};
                out.texcoords[3] = vec2f_t{0.0f, 0.0f};
                out.norm = vec3f_t{0.0f, 0.0f, -1.0f};
                return true;
            case VOXEL_FACE_SOUTH:
                out.offs[0] = local_coord_t{0, 0, 1};
                out.offs[1] = local_coord_t{1, 0, 1};
                out.offs[2] = local_coord_t{1, 1, 1};
                out.offs[3] = local_coord_t{0, 1, 1};
                out.texcoords[0] = vec2f_t{0.0f, 0.0f};
                out.texcoords[1] = vec2f_t{1.0f, 0.0f};
                out.texcoords[2] = vec2f_t{1.0f, 1.0f};
                out.texcoords[3] = vec2f_t{0.0f, 1.0f};
                out.norm = vec3f_t{0.0f, 0.0f, 1.0f};
                return true;
            case VOXEL_FACE_EAST:
                out.offs[0] = local_coord_t{1, 0, 0};
                out.offs[1] = local_coord_t{1, 1, 0};
                out.offs[2] = local_coord_t{1, 1, 1};
                out.offs[3] = local_coord_t{1, 0, 1};
                out.texcoords[0] = vec2f_t{1.0f, 0.0f};
                out.texcoords[1] = vec2f_t{1.0f, 1.0f};
                out.texcoords[2] = vec2f_t{0.0f, 1.0f};
                out.texcoords[3] = vec2f_t{0.0f, 0.0f};
                out.norm = vec3f_t{1.0f, 0.0f, 0.0f};
                return true;
            case VOXEL_FACE_WEST:
                out.offs[0] = local_coord_t{0, 0, 0};
                out.offs[1] = local_coord_t{0, 0, 1};
                out.offs[2] = local_coord_t{0, 1, 1};
                out.offs[3] = local_coord_t{0, 1, 0};
                out.texcoords[0] = vec2f_t{0.0f, 0.0f};
                out.texcoords[1] = vec2f_t{1.0f, 0.0f};
                out.texcoords[2] = vec2f_t{1.0f, 1.0f};
                out.texcoords[3] = vec2f_t{0.0f, 1.0f};
                out.norm = vec3f_t{-1.0f, 0.0f, 0.0f};
                return true;
            case VOXEL_FACE_TOP:
                out.offs[0] = local_coord_t{0, 1, 0};
                out.offs[1] = local_coord_t{0, 1, 1};
                out.offs[2] = local_coord_t{1, 1, 1};
                out.offs[3] = local_coord_t{1, 1, 0};
                out.texcoords[0] = vec2f_t{1.0f, 0.0f};
                out.texcoords[1] = vec2f_t{1.0f, 1.0f};
                out.texcoords[2] = vec2f_t{0.0f, 1.0f};
                out.texcoords[3] = vec2f_t{0.0f, 0.0f};
                out.norm = vec3f_t{0.0f, 1.0f, 0.0f};
                return true;
            case VOXEL_FACE_BOTTOM:
                out.offs[0] = local_coord_t{0, 0, 0};
                out.offs[1] = local_coord_t{1, 0, 0};
                out.offs[2] = local_coord_t{1, 0, 1};
                out.offs[3] = local_coord_t{0, 0, 1};
                out.texcoords[0] = vec2f_t{0.0f, 0.0f};
                out.texcoords[1] = vec2f_t{1.0f, 0.0f};
                out.texcoords[2] = vec2f_t{1.0f, 1.0f};
                out.texcoords[3] = vec2f_t{0.0f, 1.0f};
                out.norm = vec3f_t{0.0f, -1.0f, 0.0f};
                return true;
            default:
                return false;
        }
    }

    return false;
}

class Worker final : public mixin::NonCopyable {
public:
    Worker(const chunk_coord_t &cvec);
    virtual ~Worker() = default;

    const chunk_coord_t &get_position() const;

    // OPTIMIZATION:
    // Instead of using any thread locking mechanisms to
    // avoid race conditions we just have a cached voxels
    // so threaded workers are nicely isolated frome each other.
    void cache_push(const chunk_coord_t &cvec);
    const voxel_array_t *cache_get(const chunk_coord_t &cvec) const;

    // Returns true if:
    //  The voxel at [cpos+neighbour] is NULL_VOXEL
    //  The voxel at [cpos+neighbour] doesn't have the same draw mode
    //  The voxel at [cpos+neighbour] is equal to current voxel
    bool test(voxel_t voxel, const VoxelInfo *info, const local_coord_t &neighbour) const;

    bool completed() const;
    bool cancelled() const;
    void enqueue(thread_pool &pool);
    void cancel();

    void process();
    void finalize(entt::entity entity);

private:
    bool is_cancelled {false};
    chunk_coord_t cvec {0, 0, 0};
    std::array<voxel_array_t, NUM_CHUNK_CACHE> cache {};
    std::array<VoxelMeshBuilder, NUM_VOXEL_DRAW> builders {};
    std::shared_future<bool> future {};
};

Worker::Worker(const chunk_coord_t &cvec)
    : cvec{cvec}
{
    cache_push(cvec);
    cache_push(cvec + chunk_coord_t{0, 0, 1});
    cache_push(cvec - chunk_coord_t{0, 0, 1});
    cache_push(cvec + chunk_coord_t{0, 1, 0});
    cache_push(cvec - chunk_coord_t{0, 1, 0});
    cache_push(cvec + chunk_coord_t{1, 0, 0});
    cache_push(cvec - chunk_coord_t{1, 0, 0});
}

const chunk_coord_t &Worker::get_position() const
{
    return cvec;
}

void Worker::cache_push(const chunk_coord_t &cvec)
{
    if(const Chunk *chunk = globals::world.find_chunk(cvec)) {
        const auto id = get_chunk_cache_id(this->cvec, cvec);
        cache[id] = chunk->voxels;
    }
}

const voxel_array_t *Worker::cache_get(const chunk_coord_t &cvec) const
{
    const auto id = get_chunk_cache_id(this->cvec, cvec);
    return &cache[id];
}

bool Worker::test(voxel_t voxel, const VoxelInfo *info, const local_coord_t &neighbour) const
{
    const auto id = get_voxel_id(voxel);
    const auto state = get_voxel_state(voxel);
    const auto vvec = coord::to_voxel(cvec, neighbour);

    if(info->states.size() <= state)
        return false;
    const VoxelInfo::State &stateinfo = info->states[state];

    if(const voxel_array_t *chunk = cache_get(coord::to_chunk(vvec))) {
        const auto nvoxel = chunk->at(coord::to_index(coord::to_local(vvec)));
        const auto nid = get_voxel_id(nvoxel);
        const auto nstate = get_voxel_state(nvoxel);

        if(nid == NULL_VOXEL_ID) {
            // Neighbouring NULL_VOXEL
            return true;
        }

        if(nid == id && nstate == state) {
            // Neighbouring its kin
            return false;
        }

        if(const VoxelInfo *n_info = vdef::find(nid)) {
            if(n_info->shape == VOXEL_SHAPE_CUBE) {
                if(n_info->states.size() >= nstate)
                    return false;
                const auto &nstateinfo = n_info->states[nstate];

                if(stateinfo.draw != nstateinfo.draw) {
                    // FIXME: this should be a thing only when
                    // the voxel is surrounded by non-NULL voxels
                    return true;
                }
            }

            return false; // UNDONE
        }
    }

    // Assume NULL_VOXEL_ID
    return true;
}

inline bool Worker::completed() const
{
    return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

inline bool Worker::cancelled() const
{
    return is_cancelled;
}

inline void Worker::enqueue(thread_pool &pool)
{
    future = pool.submit(std::bind(&Worker::process, this));
}

inline void Worker::cancel()
{
    is_cancelled = true;
}

void Worker::process()
{
    const voxel_array_t *chunk = cache_get(cvec);

    if(!chunk) {
        spdlog::error("What the fuck?");
        return;
    }

    for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto voxel = chunk->at(i);
        const auto id = get_voxel_id(voxel);
        const auto state = get_voxel_state(voxel);

        if(id == NULL_VOXEL_ID) {
            // Air voxels aren't meshable
            continue;
        }

        if(VoxelInfo *info = vdef::find(id)) {
            if(info->shape == VOXEL_SHAPE_NONE) {
                // Shapeless voxels aren't meshable
                continue;
            }

            if(state >= info->states.size())
                continue;
            auto &stateinfo = info->states[state];

            if(stateinfo.draw == VOXEL_DRAW_NODRAW) {
                // Invisible voxels aren't meshable
                continue;
            }

            const auto lvec = coord::to_local(i);
            const auto vvec = coord::to_voxel(cvec, lvec);

            for(voxel_face_t face = 0; face < NUM_VOXEL_FACE; ++face) {
                if(is_cancelled) {
                    builders.fill(VoxelMeshBuilder{});
                    return;
                }

                if(stateinfo.textures[face].empty()) {
                    // Face not present
                    continue;
                }

                const AtlasTexture *atex = nullptr;

                if(stateinfo.cache[face] == UINT32_MAX) {
                    if((atex = atlas::find(stateinfo.textures[face]))) {
                        stateinfo.cache[face] = atex->texture;
                    }
                }
                else {
                    // The texture is cached, there's no need
                    // to perform an expensive hashmap search
                    atex = atlas::find(stateinfo.cache[face]);
                }

                if(atex) {
                    const auto dir = get_face_direction(face);

                    // The get_face_direction function returns {0, 0, 0}
                    // for non-axis-aligned faces (for instance for cross faces
                    // for a foliage voxel), and those are culled using a different algorithm.
                    if((dir.x || dir.y || dir.z) && !test(voxel, info, lvec + dir)) {
                        // Face culled
                        continue;
                    }

                    if(VoxelFaceQuad quad = {}; get_face_quad(quad, face, info->shape)) {
                        std::array<Vertex, 4> vertices = {};

                        for(size_t i = 0; i < vertices.size(); ++i) {
                            vertices[i].position = vec3f_t{lvec + quad.offs[i]};
                            vertices[i].normal = quad.norm;
                            vertices[i].texcoord = quad.texcoords[i] * atex->max_uv;
                            vertices[i].texture = atex->texture;
                        }

                        builders[stateinfo.draw].push_back(vertices[0], vertices[1], vertices[2], vertices[3]);
                    }
                }
            }
        }
    }
}

void Worker::finalize(entt::entity entity)
{
    auto &mc = globals::world.registry.get_or_emplace<VoxelMeshComponent>(entity);

    for(size_t i = 0; i < NUM_VOXEL_DRAW; ++i) {
        Mesh &mesh = mc.meshes[i];

        const VoxelMeshBuilder &builder = builders[i];

        if(builder.empty()) {
            mesh.triangles = 0;
            mesh.cmd.set(GL_TRIANGLES, 0, 0, 0, 0);
            mesh.vbo.destroy();
            continue;
        }

        if(!mesh.vbo.get()) {
            mesh.vbo.create();
        }

        mesh.triangles = builder.size() / 3;
        mesh.vbo.resize(builder.bytes(), builder.data(), GL_STATIC_DRAW);
        mesh.cmd.set(GL_TRIANGLES, builder.size(), 1, 0, 0);
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
static std::unordered_map<chunk_coord_t, std::unique_ptr<Worker>> workers = {};

static void on_chunk_create(const ChunkCreateEvent &event)
{
    event.world->registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);
}

static void on_chunk_remove(const ChunkRemoveEvent &event)
{
    const auto it = workers.find(event.cvec);
    if(it == workers.cend())
        return;
    it->second->cancel();
}

static void onvoxel_update(const VoxelUpdateEvent &event)
{
    event.world->registry.emplace_or_replace<NeedsMeshingComponent>(event.chunk->entity);

    // OPTIMIZATION:
    // Old code used to queue ALL existing neighbours
    // for meshing which introduced a set of performance
    // issues. Now we only queue neighbours if it's strictly
    // necessary. In other words, putting a voxel right
    // smack in the middle of a chunk is as fast as it can get.
    std::unordered_set<chunk_coord_t> neighbours = {};

    for(int dim = 0; dim < 3; ++dim) {
        chunk_coord_t offset = chunk_coord_t{0, 0, 0};
        offset[dim] = 1;

        if(event.lvec[dim] == 0) {
            neighbours.emplace(event.cvec - offset);
            continue;
        }

        if(event.lvec[dim] == CHUNK_SIZE - 1) {
            neighbours.emplace(event.cvec + offset);
            continue;
        }
    }

    for(const chunk_coord_t &ncvec : neighbours) {
        if(const Chunk *nchunk = globals::world.find_chunk(ncvec)) {
            globals::world.registry.emplace_or_replace<NeedsMeshingComponent>(nchunk->entity);
        }
    }
}

void voxel_mesher::init()
{
    workers.clear();

    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<VoxelUpdateEvent>().connect<&onvoxel_update>();
}

void voxel_mesher::deinit()
{
    for(auto &it : workers)
        it.second->cancel();
    workers_pool.wait_for_tasks();
    workers.clear();
}

void voxel_mesher::update()
{
    size_t finalized = 0;
    auto worker = workers.begin();
    while(worker != workers.cend()) {
        if(worker->second->completed()) {
            if(const Chunk *chunk = globals::world.find_chunk(worker->second->get_position())) {
                worker->second->finalize(chunk->entity);
                ++finalized;
            }

            worker = workers.erase(worker);

            if(finalized >= MESHER_TASKS_PER_FRAME)
                break;
            continue;
        }

        ++worker;
    }

    size_t queued = 0;
    const auto group = globals::world.registry.group<NeedsMeshingComponent>(entt::get<ChunkComponent>);
    for(const auto [entity, chunk] : group.each()) {
        if(workers.find(chunk.cvec) == workers.cend()) {
            globals::world.registry.remove<NeedsMeshingComponent>(entity);

            auto &worker = workers.emplace(chunk.cvec, std::make_unique<Worker>(chunk.cvec)).first->second;

            // UNDONE: if the player is within one chunk radius,
            // we might want to simply go with blocking worker->process().
            worker->enqueue(workers_pool);

            ++queued;
        }

        if(queued >= MESHER_TASKS_PER_FRAME)
            break;
        continue;
    }
}
