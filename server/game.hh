#ifndef BA8A0FD8_01E0_478F_A442_4A08436EF581
#define BA8A0FD8_01E0_478F_A442_4A08436EF581

class ServerWorld;

namespace server_game
{
void init(void);
void init_late(void);
void shutdown(void);
void fixed_update(ServerWorld* world);
void fixed_update_late(void);
} // namespace server_game

#endif /* BA8A0FD8_01E0_478F_A442_4A08436EF581 */
