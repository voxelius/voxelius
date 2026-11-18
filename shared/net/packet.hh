#ifndef CD2A5EA0_1394_4165_9926_3411C2165A5B
#define CD2A5EA0_1394_4165_9926_3411C2165A5B

class ReadBuffer;
class WriteBuffer;

namespace packet
{
enum packet_type : std::uint16_t {
    STATUS_REQUEST = 0,
    STATUS_RESPONSE = 1,
    AUTH_REQUEST,
    AUTH_CHALLENGE,
    AUTH_RESPONSE,
    AUTH_ADMISSION,
    SESSION_DISCONNECT,
    WORLD_REQUEST_CHUNK,
    WORLD_CHUNK_BLOCKS,
    WORLD_CHUNK_BIOMES,
    WORLD_SET_BLOCK,
    WORLD_TIMINGS,
    CVAR_SERVER_VARS,
    PLAYER_ATTACK_E,
    PLAYER_ATTACK_B,
    PLAYER_INTERACT_E,
    PLAYER_INTERACT_B,
    PLAYER_MOVE_DATA,
    ENTITY_SPAWN,
    ENTITY_PATCH,
    ENTITY_REMOVE,
    ENTITY_CLIENT,
    CVAR_SERVER_CMDS,
    CVAR_COMMAND,
    MSG_LOG,
    WORLD_CHUNK_LIGHT,
    WORLD_WEATHER,
    WORLD_INFO,
};
} // namespace packet

namespace packet
{
template<packet_type T>
struct Base {
    constexpr static packet_type TYPE = T;

    ENetPeer* peer { nullptr };
};
} // namespace packet

namespace packet
{
struct Status_Request;
struct Status_Response;
} // namespace packet

namespace packet
{
struct Auth_Request;
struct Auth_Challenge;
struct Auth_Response;
struct Auth_Admission;
} // namespace packet

namespace packet
{
struct Session_Disconnect;
} // namespace packet

namespace packet
{
struct World_Request;
struct World_Blocks;
struct World_Biomes;
struct World_Light;
struct World_SetBlock;
struct World_Timings;
struct World_Info;
struct World_Weather;
} // namespace packet

namespace packet
{
struct Cvar_ServerVars;
struct Cvar_ServerCmds;
struct Cvar_Command;
} // namespace packet

namespace packet
{
struct Msg_Log;
} // namespace packet

namespace packet
{
struct Player_AttackE;
struct Player_AttackB;
struct Player_InteractE;
struct Player_InteractB;
struct Player_MoveData;
} // namespace packet

namespace packet
{
struct Entity_Spawn;
struct Entity_Patch;
struct Entity_Remove;
struct Entity_Client;
} // namespace packet

#endif /* CD2A5EA0_1394_4165_9926_3411C2165A5B */
