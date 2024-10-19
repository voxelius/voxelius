// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once

// Staging subsystem contains early and
// temporary features that don't have a
// good place to be permanently implemented yet

namespace staging
{
void init(void);
void init_late(void);
void deinit(void);
void update(void);
void update_late(void);
} // namespace staging
