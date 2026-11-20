#ifndef D9B8669A_B6E3_4F38_A689_3AC3A988CE25
#define D9B8669A_B6E3_4F38_A689_3AC3A988CE25

class Identifier;
class World;

namespace client_game
{
void init_world(void);
void open_world(const Identifier& id);
std::span<const std::shared_ptr<World>> worlds(void);
} // namespace client_game

namespace client_game
{
void init(void);
void init_late(void);
void shutdown(void);
void update(void);
void update_late(void);
void fixed_update(void);
void fixed_update_late(void);
void layout(void);
void render(void);
} // namespace client_game

#endif /* D9B8669A_B6E3_4F38_A689_3AC3A988CE25 */
