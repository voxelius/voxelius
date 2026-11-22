#ifndef F3A6C8B2_9D4E_4F1A_8C7B_2E5D6A9F0B13
#define F3A6C8B2_9D4E_4F1A_8C7B_2E5D6A9F0B13

#include "shared/coord.hh"

class World;

namespace entity_loader
{
void save(World* world, const ChunkPos& pos, std::span<const entt::entity> entities);
void load(World* world, const ChunkPos& pos);
} // namespace entity_loader

#endif /* F3A6C8B2_9D4E_4F1A_8C7B_2E5D6A9F0B13 */
