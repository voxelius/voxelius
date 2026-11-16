#ifndef E9E2840F_0C1E_4B74_987A_B592B5FD6A5F
#define E9E2840F_0C1E_4B74_987A_B592B5FD6A5F

#include "shared/entity/component.hh"

struct Head {
    static void register_component(void);

    Eigen::Vector3f angles;
    Eigen::Vector3f offset;
};

struct Head_Intr final : public Head {};
struct Head_Prev final : public Head {};

#endif /* E9E2840F_0C1E_4B74_987A_B592B5FD6A5F */
