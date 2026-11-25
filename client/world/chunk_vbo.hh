#ifndef ACA902DE_6F0F_4D0B_8BC3_A6DD5E4C8007
#define ACA902DE_6F0F_4D0B_8BC3_A6DD5E4C8007

struct ChunkMeshVertex;

namespace chunk_vbo
{
struct FreeRegion final {
    std::uint32_t base;
    std::uint32_t size;
};
} // namespace chunk_vbo

namespace chunk_vbo
{
extern GLuint handle;
extern std::size_t capacity;
extern std::vector<FreeRegion> free_list;
} // namespace chunk_vbo

namespace chunk_vbo
{
void init(void);
void shutdown(void);
} // namespace chunk_vbo

namespace chunk_vbo
{
std::uint32_t allocate(std::size_t vertex_count);
void free(std::uint32_t base, std::size_t vertex_count);
void upload(std::uint32_t base, std::span<const ChunkMeshVertex> data);
} // namespace chunk_vbo

#endif /* ACA902DE_6F0F_4D0B_8BC3_A6DD5E4C8007 */
