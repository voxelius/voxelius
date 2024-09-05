// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <string>

namespace client_chat
{
void init(void);
void init_late(void);
void update(void);
void layout(void);
} // namespace client_chat

namespace client_chat
{
void clear(void);
void send(const std::string &message);
} // namespace client_chat
