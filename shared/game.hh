#ifndef D19715ED_9763_4A9D_A415_95D786234B6C
#define D19715ED_9763_4A9D_A415_95D786234B6C

class World;

namespace shared_game
{
void init(void);
void init_late(void);
void shutdown(void);
void fixed_update(World* world);
void fixed_update_late(void);
} // namespace shared_game

#endif /* D19715ED_9763_4A9D_A415_95D786234B6C */
