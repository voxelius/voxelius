// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <string>

namespace server_chat
{
void init(void);
void send(const std::string &message);
} // namespace server_chat
