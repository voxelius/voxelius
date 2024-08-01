// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_INPUT_MOUSE_HH
#define CLIENT_INPUT_MOUSE_HH

namespace mouse
{
extern bool enable_raw_input;
extern float sensitivity;
} // namespace mouse

namespace mouse
{
void init(void);
void update_late(void);
} // namespace mouse

#endif /* CLIENT_INPUT_MOUSE_HH */
