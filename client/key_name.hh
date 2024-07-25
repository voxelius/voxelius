// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_KEY_NAME_HH
#define CLIENT_KEY_NAME_HH
#include <string>

namespace key_name
{
void init(void);
} // namespace key_name

namespace key_name
{
const std::string &get(int key_code);
} // namespace key_name

#endif /* CLIENT_KEY_NAME_HH */
