// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GAMEUI_PROGRESS_HH
#define CLIENT_GAMEUI_PROGRESS_HH
#include <string>

namespace progress
{
void init(void);
void layout(void);
} // progress

namespace progress
{
void set_title(const std::string &title);
void set_subtitle(const std::string &subtitle);
void set(float value);
} // namespace progress

#endif /* CLIENT_GAMEUI_PROGRESS_HH */
