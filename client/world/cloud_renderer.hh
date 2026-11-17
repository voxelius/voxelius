#ifndef F2B7C6E2_9A85_4D3F_8E6B_7F3A5B0D4C96
#define F2B7C6E2_9A85_4D3F_8E6B_7F3A5B0D4C96

namespace cloud_renderer
{
constexpr static unsigned MAX_RANGE_CHUNKS = 256;

// The depth range the deck is drawn with; the composite compares the ground's own range against it
extern float z_near;
extern float z_far;
} // namespace cloud_renderer

namespace cloud_renderer
{
void init(void);
void shutdown(void);

// Drops the deck and the field frame it was built against; used when the weather it belongs to is wiped
void reset(void);

void render(void);
} // namespace cloud_renderer

#endif /* F2B7C6E2_9A85_4D3F_8E6B_7F3A5B0D4C96 */
