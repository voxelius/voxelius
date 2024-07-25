// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_UI_PROGRESS_HH
#define CLIENT_UI_PROGRESS_HH
#include <string>

namespace ui::progress
{
void init(void);
void layout(void);
} // ui::progress

namespace ui::progress
{
void set_title(const std::string &title);
void set_subtitle(const std::string &subtitle);
void set_progress(float progress);
} // namespace ui::progress

#endif /* CLIENT_UI_PROGRESS_HH */
