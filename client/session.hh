// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_SESSION_HH
#define CLIENT_SESSION_HH
#include <cstdint>
#include <enet/enet.h>
#include <shared/protocol.hh>
#include <string>

namespace session
{
extern ENetPeer *peer;
extern std::uint16_t session_id;
extern std::uint64_t tick_time;
extern std::string username;
} // namespace session

namespace session
{
void init(void);
void connect(const std::string &host, std::uint16_t port, const std::string &username);
void disconnect(const std::string &reason);
} // namespace session

#endif /* CLIENT_SESSION_HH */
