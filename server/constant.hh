#ifndef FE82A250_A03D_48F6_8A3F_759CAE21E519
#define FE82A250_A03D_48F6_8A3F_759CAE21E519

#include "shared/constant.hh"

namespace constant
{
constexpr static std::size_t REGION_SIZE = 8;
constexpr static std::size_t REGION_SIZE_LOG2 = 3;
constexpr static std::size_t REGION_AREA = REGION_SIZE * REGION_SIZE;
constexpr static std::size_t REGION_VOLUME = REGION_SIZE * REGION_SIZE * REGION_SIZE;
} // namespace constant

#endif /* FE82A250_A03D_48F6_8A3F_759CAE21E519 */
