#ifndef C912279E_0DB3_4252_892A_DE707A272C20
#define C912279E_0DB3_4252_892A_DE707A272C20

#include "shared/coord.hh"
#include "shared/world/biome.hh"

namespace utils
{
biome_realm realm(ChunkPos::value_type y);
biome_realm realm_of_y(float by);
} // namespace utils

#endif /* C912279E_0DB3_4252_892A_DE707A272C20 */
