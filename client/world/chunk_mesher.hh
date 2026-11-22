#ifndef BA62AFBE_7BCB_4C8F_9F70_6F8A4086C8F5
#define BA62AFBE_7BCB_4C8F_9F70_6F8A4086C8F5

#include "shared/coord.hh"

namespace chunk_mesher
{
void init(void);
void reset(void);
void update(void);
} // namespace chunk_mesher

namespace chunk_mesher
{
// Chunks that still wait for a mesh: queued and in flight together. The world
// loader asks for as many chunks as this stays low, so a client that cannot
// keep up does not pull the whole world into memory at once.
std::size_t backlog(void);
} // namespace chunk_mesher

namespace chunk_mesher
{
bool is_pending(const ChunkPos& cpos);
} // namespace chunk_mesher

#endif /* BA62AFBE_7BCB_4C8F_9F70_6F8A4086C8F5 */
