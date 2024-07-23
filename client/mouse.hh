// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_MOUSE_HH
#define CLIENT_MOUSE_HH

namespace mouse
{
extern bool raw_input;
extern float sensitivity;
} // namespace mouse

namespace mouse
{
void init(void);
void init_late(void);
void update_late(void);
} // namespace mouse

#endif /* CLIENT_MOUSE_HH */
