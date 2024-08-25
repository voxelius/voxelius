// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once

namespace mouse
{
extern bool enable_raw_input;
extern float sensitivity;
} // namespace mouse

namespace mouse
{
void init(void);
void update_late(void);
} // namespace mouse
