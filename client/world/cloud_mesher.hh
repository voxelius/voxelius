#ifndef D6A2C4E8_7B31_4F59_8A26_C3E5B7D9F102
#define D6A2C4E8_7B31_4F59_8A26_C3E5B7D9F102

#include "client/world/cloud_field.hh"

// Color plus the alpha the layer asks for, kept as three
// floats and a scalar: Eigen over-aligns a Vector4f to
// sixteen bytes, and that padding would break the vertex layout
struct CloudColor final {
    Eigen::Vector3f rgb;
    float alpha;
};

struct CloudVertex final {
    Eigen::Vector3f position;
    Eigen::Vector3f normal;
    CloudColor color;
};

struct CloudWindow final {
    std::int32_t min_x;
    std::int32_t min_z;
    std::size_t width;
    std::size_t depth;
};

struct CloudBatch final {
    std::size_t offset;
    std::size_t count;
    Eigen::Vector2f center; // Window center this batch was meshed around, in the frame of its vertices
    float fade;             // Outer edge of the deck; zero for a ring inside it, which does not fade out
    float blend_start;      // Outer edge of the ring, where it fades into the coarser one; a zero width means no fade
    float blend_width;
    float depth_bias; // Pushes the ring back in depth, so two rings that overlap at the same height do not fight over the pixel
};

// One build's worth of geometry; a worker
// fills it and the main thread uploads it
struct CloudMesh final {
    std::vector<CloudVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<CloudBatch> batches;
};

namespace cloud_mesher
{
// Fills the mesh with the geometry of every group. Groups are drawn farthest
// first, which is the order the renderer needs for blending.
void build(const CloudInput& input, std::span<const CloudGroup> groups, CloudMesh& mesh);
} // namespace cloud_mesher

#endif /* D6A2C4E8_7B31_4F59_8A26_C3E5B7D9F102 */
