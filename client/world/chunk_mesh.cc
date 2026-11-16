#include "client/pch.hh"

#include "client/world/chunk_mesh.hh"

#include "client/world/chunk_vbo.hh"

static std::uint32_t s_slot_cap;
static std::vector<std::uint32_t> s_free_slots;

static std::uint32_t pack_10_10_10(const Eigen::Vector3f& value_16ths, std::int32_t bias)
{
    auto bx = bias + static_cast<std::int32_t>(std::lround(value_16ths.x()));
    auto by = bias + static_cast<std::int32_t>(std::lround(value_16ths.y()));
    auto bz = bias + static_cast<std::int32_t>(std::lround(value_16ths.z()));
    auto px = static_cast<std::uint32_t>(std::clamp(bx, INT32_C(0), INT32_C(1023)));
    auto py = static_cast<std::uint32_t>(std::clamp(by, INT32_C(0), INT32_C(1023)));
    auto pz = static_cast<std::uint32_t>(std::clamp(bz, INT32_C(0), INT32_C(1023)));
    return px | (py << 10U) | (pz << 20U);
}

static std::uint32_t pack_unorm8(float value)
{
    return static_cast<std::uint32_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f)) & 0xFFU;
}

std::uint32_t ChunkMesh_Vertex::pack_1(const Eigen::Vector3f& position_16ths)
{
    return pack_10_10_10(position_16ths, INT32_C(16));
}

std::uint32_t ChunkMesh_Vertex::pack_2(const Eigen::Vector2f& uv, std::uint32_t mask_frame)
{
    std::uint32_t result = 0;
    result |= pack_unorm8(uv.x());
    result |= pack_unorm8(uv.y()) << 8U;
    result |= (mask_frame & 0xFFFFU) << 16U;
    return result;
}

std::uint32_t ChunkMesh_Vertex::pack_3(std::uint32_t albedo_strip, std::uint32_t frame_offset)
{
    std::uint32_t result = 0;
    result |= (albedo_strip & 0xFFFFU);
    result |= (frame_offset & 0xFFFFU) << 16U;
    return result;
}

std::uint32_t ChunkMesh_Vertex::pack_4(std::uint32_t ao, float shade, bool animated, const Eigen::Vector3f& tint_rgb,
    block_light_type sky_light, block_light_type block_light)
{
    auto r = static_cast<std::uint32_t>(std::lround(std::clamp(tint_rgb.x(), 0.0f, 1.0f) * 31.0f)) & 0x1FU;
    auto g = static_cast<std::uint32_t>(std::lround(std::clamp(tint_rgb.y(), 0.0f, 1.0f) * 63.0f)) & 0x3FU;
    auto b = static_cast<std::uint32_t>(std::lround(std::clamp(tint_rgb.z(), 0.0f, 1.0f) * 31.0f)) & 0x1FU;
    auto s = static_cast<std::uint32_t>(std::lround(std::clamp(shade, 0.0f, 1.0f) * 31.0f)) & 0x1FU;

    std::uint32_t result = 0;
    result |= ao & 0x03U;
    result |= (static_cast<std::uint32_t>(sky_light) & 0x0FU) << 3U;
    result |= (static_cast<std::uint32_t>(block_light) & 0x0FU) << 7U;
    result |= s << 11U;
    result |= r << 16U;
    result |= g << 21U;
    result |= b << 27U;

    if(animated) {
        result |= ANIMATED_BIT;
    }

    return result;
}

ChunkMesh_Part::~ChunkMesh_Part(void)
{
    if(count) {
        chunk_vbo::free(base_vertex, count);
    }
}

ChunkMesh_Part::ChunkMesh_Part(ChunkMesh_Part&& other)
    : vertices(std::move(other.vertices)), count(other.count), base_vertex(other.base_vertex)
{
    other.count = 0;
    other.base_vertex = 0;
}

ChunkMesh_Part& ChunkMesh_Part::operator=(ChunkMesh_Part&& other)
{
    if(this == &other) {
        return *this;
    }

    if(count) {
        chunk_vbo::free(base_vertex, count);
    }

    count = 0;
    base_vertex = 0;

    vertices = std::move(other.vertices);
    count = other.count;
    base_vertex = other.base_vertex;

    other.count = 0;
    other.base_vertex = 0;

    return *this;
}

ChunkMesh::ChunkMesh(void)
{
    if(s_free_slots.size()) {
        slot = s_free_slots.back();
        s_free_slots.pop_back();
    }
    else {
        slot = s_slot_cap;
        s_slot_cap += 1;
    }
}

ChunkMesh::~ChunkMesh(void)
{
    if(slot < std::numeric_limits<std::uint32_t>::max()) {
        s_free_slots.push_back(slot);
    }
}

ChunkMesh::ChunkMesh(ChunkMesh&& other)
    : slot(other.slot), opaque(std::move(other.opaque)), alpha(std::move(other.alpha)), fluid(std::move(other.fluid))
{
    other.slot = std::numeric_limits<std::uint32_t>::max();
}

ChunkMesh& ChunkMesh::operator=(ChunkMesh&& other)
{
    if(this == &other) {
        return *this;
    }

    slot = other.slot;
    opaque = std::move(other.opaque);
    alpha = std::move(other.alpha);
    fluid = std::move(other.fluid);

    other.slot = std::numeric_limits<std::uint32_t>::max();

    return *this;
}
