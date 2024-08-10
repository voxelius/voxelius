// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_MESSAGE_BOX_HH
#define CLIENT_MESSAGE_BOX_HH
#include <string>

using MessageBoxAction = void(*)(void);

namespace message_box
{
void init(void);
void layout(void);
void reset(void);
} // namespace message_box

namespace message_box
{
void set_title(const std::string &title);
void set_subtitle(const std::string &subtitle);
void add_button(const std::string &text, const MessageBoxAction &action);
} // namespace message_box

#endif /* CLIENT_MESSAGE_BOX_HH */
