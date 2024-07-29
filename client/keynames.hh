// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_KEYNAMES_HH
#define CLIENT_KEYNAMES_HH
#include <string>

namespace keynames
{
void init(void);
void deinit(void);
} // namespace keynames

namespace keynames
{
const std::string &get(int key_code);
} // namespace keynames

#endif /* CLIENT_KEYNAMES_HH */
