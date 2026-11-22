#ifndef D9B8669A_B6E3_4F38_A689_3AC3A988CE25
#define D9B8669A_B6E3_4F38_A689_3AC3A988CE25

class World;

namespace packet
{
struct World_Info;
} // namespace packet

namespace client_game
{
void open_world(const packet::World_Info& packet);
void init_world(void);
} // namespace client_game

namespace client_game
{
void init(void);
void init_late(void);
void shutdown(void);
void fixed_update(World* world);
void fixed_update_late(void);
void update(void);
void update_late(void);
void render(void);
void layout(void);
} // namespace client_game

#endif /* D9B8669A_B6E3_4F38_A689_3AC3A988CE25 */
