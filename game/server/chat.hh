// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <string>

struct Session;

namespace server_chat
{
void init(void);
void broadcast(const std::string &message);
void broadcast(const std::string &message, const std::string &sender);
void send(Session *session, const std::string &message);
void send(Session *session, const std::string &message, const std::string &sender);
} // namespace server_chat
