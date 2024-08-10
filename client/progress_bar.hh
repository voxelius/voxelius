// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_PROGRESS_BAR_HH
#define CLIENT_PROGRESS_BAR_HH
#include <string>

using ProgressBarAction = void(*)(void);

namespace progress_bar
{
void init(void);
void layout(void);
} // progress_bar

namespace progress_bar
{
void reset(void);
void set_title(const std::string &title);
void set_button(const std::string &text, const ProgressBarAction &action);
void set_progress(float value);
} // namespace progress_bar

#endif /* CLIENT_PROGRESS_BAR_HH */
