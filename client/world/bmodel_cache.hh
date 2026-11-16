#ifndef E58051D6_68EC_495E_92C9_545EBD9CD3E9
#define E58051D6_68EC_495E_92C9_545EBD9CD3E9

#include "shared/world/block.hh"
#include "shared/world/tint.hh"

struct CachedBlockModel_Quad final {
    std::array<Eigen::Vector3f, 4> positions;
    std::array<Eigen::Vector2f, 4> uvs;
    std::uint32_t packed_normal;
    std::uint32_t albedo_strip;
    std::uint32_t albedo_frames;
    std::uint32_t mask_frame;
    tint_id_type tint;
    bool animated;
    bool shade;
};

struct CachedBlockModel final {
    std::vector<CachedBlockModel_Quad> unculled_quads;
    std::array<std::vector<CachedBlockModel_Quad>, 6> face_quads;
    std::array<bool, 6> fully_covered;
};

namespace bmodel_cache
{
void init_late(void);
void shutdown(void);
} // namespace bmodel_cache

namespace bmodel_cache
{
const CachedBlockModel* find(block_id_type id);
block_face face_from_normal(std::uint32_t packed_normal);
} // namespace bmodel_cache

#endif /* E58051D6_68EC_495E_92C9_545EBD9CD3E9 */
