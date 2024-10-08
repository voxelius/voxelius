// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <string>

using ProgressBarAction = void(*)(void);

namespace progress
{
void init(void);
void layout(void);
} // progress

namespace progress
{
void reset(void);
void set_title(const std::string &title);
void set_button(const std::string &text, const ProgressBarAction &action);
} // namespace progress
