#ifndef E4130E55_D817_41FD_86F5_99BE2CB4D74B
#define E4130E55_D817_41FD_86F5_99BE2CB4D74B

struct MoveData final {
    Eigen::Vector3f wishdir;
};

struct JumpCooldown final {
    std::uint32_t ticks;
};

namespace pmove
{
void fixed_update(entt::registry& registry);
} // namespace pmove

#endif /* E4130E55_D817_41FD_86F5_99BE2CB4D74B */
