#ifndef F2B7C6E2_9A85_4D3F_8E6B_7F3A5B0D4C96
#define F2B7C6E2_9A85_4D3F_8E6B_7F3A5B0D4C96

namespace cloud_renderer
{
extern float z_near;
extern float z_far;
} // namespace cloud_renderer

namespace cloud_renderer
{
void init(void);
void shutdown(void);
void render(void);
void reset(void);
} // namespace cloud_renderer

#endif /* F2B7C6E2_9A85_4D3F_8E6B_7F3A5B0D4C96 */
