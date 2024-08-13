// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_NETWORK_HH
#define SERVER_NETWORK_HH
#include <string>

struct Session;

namespace server_network
{
extern unsigned int listen_port;
extern unsigned int incoming_bandwidth;
extern unsigned int outgoing_bandwidth;
} // namespace server_network

namespace server_network
{
void init(void);
void init_late(void);
void deinit(void);
void update_late(void);
} // namespace server_network

namespace server_network
{
void kick_all(const std::string &reason);
void kick(Session *session, const std::string &reason);
} // namespace server_network

#endif /* SERVER_NETWORK_HH */
