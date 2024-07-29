// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_UTIL_KEYNAME_HH
#define CLIENT_UTIL_KEYNAME_HH
#include <string>

namespace keyname
{
void init(void);
const std::string &get(int key_code);
} // namespace keyname

#endif /* CLIENT_UTIL_KEYNAME_HH */
