#ifndef BA62AFBE_7BCB_4C8F_9F70_6F8A4086C8F5
#define BA62AFBE_7BCB_4C8F_9F70_6F8A4086C8F5

#include "shared/coord.hh"

namespace chunk_mesher
{
extern std::size_t backlog;
} // namespace chunk_mesher

namespace chunk_mesher
{
void init(void);
void update(void);
void reset(void);
} // namespace chunk_mesher

namespace chunk_mesher
{
bool is_pending(const ChunkPos& cpos);
} // namespace chunk_mesher

#endif /* BA62AFBE_7BCB_4C8F_9F70_6F8A4086C8F5 */
