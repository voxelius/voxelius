#ifndef C61C83B0_E6D3_4A4F_B49E_930C332718D3
#define C61C83B0_E6D3_4A4F_B49E_930C332718D3

#include "shared/coord.hh"
#include "shared/entity/component.hh"

struct Transform {
    static void register_component(void);
    static void fixed_update(void);

    ChunkPos chunk;
    Eigen::Vector3f local;
    Eigen::Vector3f angles;
};

struct Transform_Intr final : public Transform {};
struct Transform_Prev final : public Transform {};

#endif /* C61C83B0_E6D3_4A4F_B49E_930C332718D3 */
