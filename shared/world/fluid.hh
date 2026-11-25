#ifndef B347B878_D23E_45CF_A346_EDD80070F951
#define B347B878_D23E_45CF_A346_EDD80070F951

#include "core/identifier.hh"

#include "shared/world/tint.hh"

enum fluid_gravity : unsigned {
    FLUID_GRAVITY_DOWN,
    FLUID_GRAVITY_UP,
};

using fluid_id_type = std::uint32_t;
constexpr static fluid_id_type FLUID_ID_NULL = 0;
constexpr static fluid_id_type FLUID_ID_MAX = std::numeric_limits<fluid_id_type>::max();

struct FluidPhysics final {
    float drag { 0.0f };
    float gravity_scale { 0.0f };
    float input_scale { 0.0f };
    float swim_speed { 0.0f };
    float climb_speed { 0.0f };
    float flow_scale { 0.0f };
};

struct FluidDefinition final {
    FluidDefinition(void) = default;

    fluid_gravity gravity;

    std::vector<Identifier> albedo_still;
    std::vector<Identifier> albedo_flowing;

    std::optional<Identifier> mask_still;
    std::optional<Identifier> mask_flowing;

    unsigned full_level;

    FluidPhysics physics;

    bool opaque;

    float fog_density;
    Eigen::Vector3f fog_color;

    Identifier tint_name;
    tint_id_type tint;
};

#endif /* B347B878_D23E_45CF_A346_EDD80070F951 */
