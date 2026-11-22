#ifndef B3676B99_D6A9_4E82_9B2C_24D47D79D6FC
#define B3676B99_D6A9_4E82_9B2C_24D47D79D6FC

#include "shared/entity/component.hh"

struct Velocity {
    static void register_component(void);
    static void fixed_update(entt::registry& registry);

    Eigen::Vector3f value;
};

struct Velocity_Intr final : public Velocity {};
struct Velocity_Prev final : public Velocity {};

#endif /* B3676B99_D6A9_4E82_9B2C_24D47D79D6FC */
