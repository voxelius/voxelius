// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_TITLEMSG_HH
#define SHARED_TITLEMSG_HH
#include <string>

namespace titlemsg
{
void init(void);
} // namespace titlemsg

namespace titlemsg
{
const std::string &get(void);
} // namespace titlemsg

#endif /* SHARED_TITLEMSG_HH */
