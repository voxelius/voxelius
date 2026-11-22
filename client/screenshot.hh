#ifndef A7C3E914_5B62_4D8F_9A21_C4E60B8D2F17
#define A7C3E914_5B62_4D8F_9A21_C4E60B8D2F17

namespace screenshot
{
void init(void);
} // namespace screenshot

namespace screenshot
{
bool handle_event(const SDL_Event& event);
void request(void);
void capture(void);
} // namespace screenshot

#endif /* A7C3E914_5B62_4D8F_9A21_C4E60B8D2F17 */
