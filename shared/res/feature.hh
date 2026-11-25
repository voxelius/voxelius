#ifndef C07BC2BC_979F_4F6A_9742_9C48513415F4
#define C07BC2BC_979F_4F6A_9742_9C48513415F4

#include "shared/world/block.hh"

struct FeaturePart final {
    block_id_type block;
    Eigen::Vector3i offset;
    block_tag_bit overwrite;
};

struct Feature final {
    static void register_resource(void);

    std::vector<FeaturePart> parts;
    Eigen::AlignedBox3i bounds;
};

#endif /* C07BC2BC_979F_4F6A_9742_9C48513415F4 */
