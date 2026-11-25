#ifndef AA48CB2A_BA02_4057_9B78_E42EED2EEAE5
#define AA48CB2A_BA02_4057_9B78_E42EED2EEAE5

#include "shared/coord.hh"

namespace utils
{
ChunkAlignedBox view_box(const ChunkPos& center, ChunkPos::value_type radius);
ChunkAlignedBoxXZ interest_box(const ChunkPos& center, ChunkPos::value_type radius);
} // namespace utils

#endif /* AA48CB2A_BA02_4057_9B78_E42EED2EEAE5 */
