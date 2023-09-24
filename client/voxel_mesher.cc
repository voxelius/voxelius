// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/vertex.hh>
#include <client/voxel_mesher.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>
#include <thread_pool.hpp>
#include <unordered_set>

// FIXME: forward-port packed VoxelVertex
using VoxelMeshBuilder = MeshBuilder<Vertex>;

using chunk_cache_id_t = unsigned int;
constexpr static const chunk_cache_id_t CHUNK_CACHE_ITSELF  = 0;
constexpr static const chunk_cache_id_t CHUNK_CACHE_NORTH   = 1;
constexpr static const chunk_cache_id_t CHUNK_CACHE_SOUTH   = 2;
constexpr static const chunk_cache_id_t CHUNK_CACHE_EAST    = 3;
constexpr static const chunk_cache_id_t CHUNK_CACHE_WEST    = 4;
constexpr static const chunk_cache_id_t CHUNK_CACHE_TOP     = 5;
constexpr static const chunk_cache_id_t CHUNK_CACHE_BOTTOM  = 6;
constexpr static const size_t NUM_CHUNK_CACHE = 7;

struct BakedVoxelInfo final {
    voxel_t voxel {};
    uint16_t voxel_id {};
    uint16_t voxel_state {};
    VoxelInfo *info {nullptr};
    VoxelInfo::State *stateinfo {nullptr};
};

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
            return local_coord_t(0, 0, -1);
        case VOXEL_FACE_SOUTH:
            return local_coord_t(0, 0, 1);
        case VOXEL_FACE_EAST:
            return local_coord_t(1, 0, 0);
        case VOXEL_FACE_WEST:
            return local_coord_t(-1, 0, 0);
        case VOXEL_FACE_TOP:
            return local_coord_t(0, 1, 0);
        case VOXEL_FACE_BOTTOM:
            return local_coord_t(0, -1, 0);
        default:
            return local_coord_t(0, 0, 0);
    }
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

    void greedy_face(const voxel_array_t *chunk, const BakedVoxelInfo &bake, voxel_face_t face);
    void process();

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
        const chunk_cache_id_t id = get_chunk_cache_id(this->cvec, cvec);
        cache[id] = chunk->voxels;
    }
}

const voxel_array_t *Worker::cache_get(const chunk_coord_t &cvec) const
{
    const chunk_cache_id_t id = get_chunk_cache_id(this->cvec, cvec);
    return &cache[id];
}

bool Worker::test(voxel_t voxel, const VoxelInfo *info, const local_coord_t &neighbour) const
{
    const uint16_t id = get_voxel_id(voxel);
    const uint16_t state = get_voxel_state(voxel);
    const voxel_coord_t vvec = coord::to_voxel(cvec, neighbour);

    if(info->states.size() <= state)
        return false;
    const VoxelInfo::State &stateinfo = info->states[state];

    if(const voxel_array_t *chunk = cache_get(coord::to_chunk(vvec))) {
        const voxel_t n_voxel = chunk->at(coord::to_index(coord::to_local(vvec)));
        const uint16_t n_id = get_voxel_id(n_voxel);
        const uint16_t n_state = get_voxel_state(n_voxel);

        if(n_id == NULL_VOXEL_ID) {
            // Neighbouring NULL_VOXEL
            return true;
        }

        if(n_id == id && n_state == state) {
            // Neighbouring its kin
            return false;
        }

        if(const VoxelInfo *n_info = vdef::find(n_id)) {
            if(n_info->shape == VOXEL_SHAPE_CUBE) {
                if(n_info->states.size() >= n_state)
                    return false;
                const VoxelInfo::State &n_stateinfo = n_info->states[n_state];

                if(stateinfo.draw != n_stateinfo.draw) {
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

}

static void on_chunk_create(const ChunkCreateEvent &event)
{

}

static void on_chunk_remove(const ChunkRemoveEvent &event)
{

}

static void on_voxel_update(const VoxelUpdateEvent &event)
{

}

void voxel_mesher::init()
{
    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<VoxelUpdateEvent>().connect<&on_voxel_update>();
}

void voxel_mesher::deinit()
{

}

void voxel_mesher::update()
{

}
