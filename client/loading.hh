#ifndef EE424B5E_3A4B_4CBB_9C91_2DCDC42E72B0
#define EE424B5E_3A4B_4CBB_9C91_2DCDC42E72B0

enum loading_stage {
    LOADING_IDLE = 0,
    LOADING_CONNECTING,
    LOADING_AUTHENTICATING,
    LOADING_SPAWNING,
    LOADING_CHUNKS,
};

namespace loading
{
extern loading_stage stage;
} // namespace loading

namespace loading
{
void init(void);
void shutdown(void);
void update(void);
} // namespace loading

namespace loading
{
void show_stage(loading_stage new_stage);
void attach_player(void);
void on_local_patch(void);
} // namespace loading

#endif /* EE424B5E_3A4B_4CBB_9C91_2DCDC42E72B0 */
