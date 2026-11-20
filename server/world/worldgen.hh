#ifndef EE25AA21_7F3F_4B81_B031_17C2207A048A
#define EE25AA21_7F3F_4B81_B031_17C2207A048A

#include "shared/coord.hh"

class World;

namespace worldgen
{
void init(void);
void shutdown(void);
void request(const std::shared_ptr<World>& world, const ChunkPos& pos);
bool pending(World* world, const ChunkPos& pos);
} // namespace worldgen

#endif /* EE25AA21_7F3F_4B81_B031_17C2207A048A */
