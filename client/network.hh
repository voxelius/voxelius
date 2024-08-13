// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_NETWORK_HH
#define CLIENT_NETWORK_HH
#include <shared/protocol/protocol.hh>
#include <string>

namespace client_network
{
void init(void);
void init_late(void);
void deinit(void);
void update(void);
} // namespace client_network

namespace client_network
{
void connect(const std::string &address, std::uint16_t port = protocol::DEFAULT_PORT);
void disconnect(const std::string &reason);
} // namespace client_network

#endif /* CLIENT_NETWORK_HH */
