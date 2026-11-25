#ifndef FBCA6C79_8B06_42C8_9546_3569F1A321AD
#define FBCA6C79_8B06_42C8_9546_3569F1A321AD

#include "core/identifier.hh"

struct AtlasStrip final {
    std::size_t frame_base;
    std::size_t frame_count;
    std::size_t index;
};

struct AtlasFrameGL final {
    Eigen::Vector2f uv_min;
    Eigen::Vector2f uv_max;
    std::uint32_t layer;
    std::uint32_t reserved_1;
    std::uint32_t reserved_2;
    std::uint32_t reserved_3;
};

static_assert(0x20 == sizeof(AtlasFrameGL));
static_assert(0x08 == offsetof(AtlasFrameGL, uv_max));
static_assert(0x10 == offsetof(AtlasFrameGL, layer));

struct AtlasStripGL final {
    std::uint32_t frame_base;
    std::uint32_t frame_count;
};

static_assert(0x08 == sizeof(AtlasStripGL));
static_assert(0x04 == offsetof(AtlasStripGL, frame_count));

namespace block_atlas
{
extern GLuint texture;
extern GLuint vbo_frames;
extern GLuint tbo_frames;
extern GLuint vbo_strips;
extern GLuint tbo_strips;
} // namespace block_atlas

namespace block_atlas
{
extern const AtlasStrip* stub_white;
extern const AtlasStrip* stub_black;
extern const AtlasStrip* stub_check;
} // namespace block_atlas

namespace block_atlas
{
const AtlasStrip* load(std::span<const Identifier> textures);
const AtlasStrip* find(std::span<const Identifier> textures);
} // namespace block_atlas

namespace block_atlas
{
void init(void);
void init_late(void);
void shutdown(void);
} // namespace block_atlas

#endif /* FBCA6C79_8B06_42C8_9546_3569F1A321AD */
