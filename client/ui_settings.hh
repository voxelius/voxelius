// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_UI_SETTINGS_HH
#define CLIENT_UI_SETTINGS_HH
#include <string>

namespace ui::settings
{
void init(void);
void init_late(void);
void layout(void);
} // namespace ui::settings

namespace ui::settings
{
void add(const std::string &name, int &vref);
void add(const std::string &name, bool &vref);
void add(const std::string &name, float &vref);
void add(const std::string &name, std::string &vref);
void add(const std::string &name, unsigned int &vref);
} // namespace ui::settings

#endif /* CLIENT_UI_SETTINGS_HH */
