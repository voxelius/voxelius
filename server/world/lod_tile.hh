#ifndef C8B5D3E1_7A29_4F64_9E10_B2D4C6A8F935
#define C8B5D3E1_7A29_4F64_9E10_B2D4C6A8F935

#include "shared/world/lod_tile.hh"

namespace lod_tile
{
void init(void);
void shutdown(void);
void fixed_update(void);
} // namespace lod_tile

namespace lod_tile
{
bool request(const LodKey& key, ENetPeer* peer);
void forget_peer(ENetPeer* peer);
std::size_t backlog(void);
} // namespace lod_tile

#endif /* C8B5D3E1_7A29_4F64_9E10_B2D4C6A8F935 */
