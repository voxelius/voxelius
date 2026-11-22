#ifndef F3A2B6C1_9D4E_4A7B_8C3F_2E1D5A6B7C8D
#define F3A2B6C1_9D4E_4A7B_8C3F_2E1D5A6B7C8D

class Camera;

namespace head::detail
{
void begin_frame(void);
void init_renderers(void);
} // namespace head::detail

namespace head
{
void init(void);
void init_late(void);
void shutdown(void);
bool prepare(void);
void render(void);
void present(void);
} // namespace head

#endif /* F3A2B6C1_9D4E_4A7B_8C3F_2E1D5A6B7C8D */
