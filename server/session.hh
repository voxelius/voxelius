// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_SESSION_HH
#define SERVER_SESSION_HH

struct Session final {
    std::uint16_t session_id {};
    std::uint64_t player_uid {};
    std::string username {};
    entt::entity player {};
};

namespace sessions
{
extern unsigned int max_players;
extern unsigned int num_players;
} // namespace sessions

namespace sessions
{
void init(void);
void init_late(void);
void deinit(void);
} // namespace sessions

namespace sessions
{
Session *create(ENetPeer *peer, std::uint64_t player_uid);
Session *find(std::uint16_t session_id);
Session *find(std::uint64_t player_uid);
Session *find(ENetPeer *peer);
void destroy(Session *session);
} // namespace sessions

namespace sessions
{


} // namespace sessions


/*

sessions::kick(session, "Kicked for cheating");


*/



#endif /* SERVER_SESSION_HH */
