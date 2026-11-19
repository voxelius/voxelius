#ifndef B8D2E4F6_1A37_4C59_8E02_D1F3A5B7C904
#define B8D2E4F6_1A37_4C59_8E02_D1F3A5B7C904

namespace lod_chunk_mesher
{
void init(void);
void shutdown(void);
void update(void);
} // namespace lod_chunk_mesher

namespace lod_chunk_mesher
{
// Chunks that still wait for their quads: queued and in flight together. The
// world loader asks for as many LOD chunks as this stays low.
std::size_t backlog(void);
} // namespace lod_chunk_mesher

#endif /* B8D2E4F6_1A37_4C59_8E02_D1F3A5B7C904 */
