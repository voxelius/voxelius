// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_SESSION_HH
#define CLIENT_SESSION_HH
#include <cstdint>

namespace session
{
void init(void);
void deinit(void);
} // namespace session

namespace session
{
void connect(const std::string &host, std::uint16_t port);
void disconnect(const std::string &reason);
void send_login_request(void);
void invalidate(void);
} // namespace session

#endif /* CLIENT_SESSION_HH */
