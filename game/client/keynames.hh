// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
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
