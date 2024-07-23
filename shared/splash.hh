// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_SPLASH_HH
#define SHARED_SPLASH_HH
#include <string>

namespace splash
{
void init(void);
} // namespace splash

namespace splash
{
const std::string &get(void);
} // namespace splash

#endif /* SHARED_SPLASH_HH */
