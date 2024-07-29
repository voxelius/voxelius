// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_SETTINGS_HH
#define CLIENT_GUI_SETTINGS_HH
#include <cstddef>
#include <string>

namespace settings
{
constexpr static std::size_t GENERAL            = 0x0000;
constexpr static std::size_t KEYBOARD_MOVEMENT  = 0x0001;
constexpr static std::size_t KEYBOARD_GAMEPLAY  = 0x0002;
constexpr static std::size_t KEYBOARD_MISC      = 0x0003;
constexpr static std::size_t MOUSE              = 0x0004;
constexpr static std::size_t VIDEO              = 0x0005;
constexpr static std::size_t VIDEO_GUI          = 0x0006;
constexpr static std::size_t SOUND              = 0x0007;
constexpr static std::size_t NUM_LOCATIONS      = 0x0008;
} // namespace settings

namespace settings
{
void init(void);
void init_late(void);
void deinit(void);
void layout(void);
} // namespace settings

namespace settings
{
void add_checkbox(int priority, std::size_t location, const std::string &name, bool &vref, bool tooltip);
void add_input(int priority, std::size_t location, const std::string &name, int &vref, bool tooltip);
void add_input(int priority, std::size_t location, const std::string &name, std::string &vref, bool tooltip, bool allow_whitespace);
void add_input(int priority, std::size_t location, const std::string &name, unsigned int &vref, bool tooltip);
void add_key_binding(int priority, std::size_t location, const std::string &name, int &vref);
void add_language_select(int priority, std::size_t location, const std::string &name);
void add_slider(int priority, std::size_t location, const std::string &name, float &vref, float min, float max, bool tooltip, const char *format = nullptr);
void add_slider(int priority, std::size_t location, const std::string &name, int &vref, int min, int max, bool tooltip);
void add_slider(int priority, std::size_t location, const std::string &name, unsigned int &vref, unsigned int min, unsigned int max, bool tooltip);
} // namespace settings

#endif /* CLIENT_GUI_SETTINGS_HH */
