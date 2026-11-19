#ifndef D5E7A9C3_1F42_4B68_8A90_C2B4D6E8F103
#define D5E7A9C3_1F42_4B68_8A90_C2B4D6E8F103

namespace packet
{
struct World_Request;
} // namespace packet

struct Session;

namespace streaming
{
void init(void);
void shutdown(void);
} // namespace streaming

namespace streaming
{
void fixed_update_late(void);
void subscribe(Session& session, const packet::World_Request& packet);
} // namespace streaming

#endif /* D5E7A9C3_1F42_4B68_8A90_C2B4D6E8F103 */
