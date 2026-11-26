#ifndef B7A1C3E5_F9B0_D4C2_E8F6_A0B2D4F6A8C1
#define B7A1C3E5_F9B0_D4C2_E8F6_A0B2D4F6A8C1

#include "shared/entity/component.hh"

struct Collision final {
    static void register_component(void);

    Eigen::AlignedBox3f local_box;
    float step_height;
};

#endif /* B7A1C3E5_F9B0_D4C2_E8F6_A0B2D4F6A8C1 */
