// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_KEYBOARD_HH
#define CLIENT_KEYBOARD_HH

namespace keyboard
{
extern int bind_move_forward;
extern int bind_move_back;
extern int bind_move_left;
extern int bind_move_right;
extern int bind_move_up;
extern int bind_move_down;
} // namespace keyboard

namespace keyboard
{
void init(void);
void update(void);
} // namespace keyboard

#endif /* CLIENT_KEYBOARD_HH */
