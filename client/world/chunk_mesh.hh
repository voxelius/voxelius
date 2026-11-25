#ifndef AD0ACA2E_153B_4EC9_A474_9E7B117A757C
#define AD0ACA2E_153B_4EC9_A474_9E7B117A757C

#include "shared/world/block.hh"

struct ChunkMeshVertex final {
    static constexpr std::uint32_t ANIMATED_BIT = 1U << 2U;

    static std::uint32_t pack_1(const Eigen::Vector3f& position_16ths);
    static std::uint32_t pack_2(const Eigen::Vector2f& uv, std::uint32_t mask_frame);
    static std::uint32_t pack_3(std::uint32_t albedo_strip, std::uint32_t frame_offset);
    static std::uint32_t pack_4(std::uint32_t ao, float shade, bool animated, const Eigen::Vector3f& tint_rgb, block_light_type sky_light,
        block_light_type block_light);

    std::uint32_t data_1; // --ZZZZZZZZZZYYYYYYYYYYXXXXXXXXXX
    std::uint32_t data_2; // MMMMMMMMMMMMMMMMVVVVVVVVUUUUUUUU
    std::uint32_t data_3; // FFFFFFFFFFFFFFFFTTTTTTTTTTTTTTTT
    std::uint32_t data_4; // CCCCCCCCCCCCCCCCSSSSSBBBBSSSSAAF
    std::uint32_t data_5; // Index into u_ChunkPositions TBO
};

struct ChunkMeshPart final {
    ChunkMeshPart(void) = default;
    ~ChunkMeshPart(void);

    ChunkMeshPart(const ChunkMeshPart& other) = delete;
    ChunkMeshPart& operator=(const ChunkMeshPart& other) = delete;

    ChunkMeshPart(ChunkMeshPart&& other);
    ChunkMeshPart& operator=(ChunkMeshPart&& other);

    std::vector<ChunkMeshVertex> vertices;
    std::uint32_t count { 0 };
    std::uint32_t base_vertex { 0 };
};

struct ChunkMesh final {
    ChunkMesh(void);
    ~ChunkMesh(void);

    ChunkMesh(const ChunkMesh& other) = delete;
    ChunkMesh& operator=(const ChunkMesh& other) = delete;

    ChunkMesh(ChunkMesh&& other);
    ChunkMesh& operator=(ChunkMesh&& other);

    std::uint32_t slot;
    ChunkMeshPart opaque;
    ChunkMeshPart alpha;
    ChunkMeshPart fluid;
};

struct ChunkMesh_DirtyMarker final {};

#endif /* AD0ACA2E_153B_4EC9_A474_9E7B117A757C */
