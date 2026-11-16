#ifndef C51A3568_618C_439C_9766_A6E101E01AAD
#define C51A3568_618C_439C_9766_A6E101E01AAD

#include "shared/coord.hh"

namespace chunk_visibility
{
void fixed_update_late(void);
} // namespace chunk_visibility

namespace chunk_visibility
{
// Tells the loader that one of its requests arrived. Requests that are still
// unanswered are what keeps it from asking for more, so the client never ends
// up flying ahead of the chunks it already ordered.
void note_received(const ChunkPos& cpos);
} // namespace chunk_visibility

#endif /* C51A3568_618C_439C_9766_A6E101E01AAD */
