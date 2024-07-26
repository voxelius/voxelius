// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_DEBUG_HH
#define CLIENT_DEBUG_HH

namespace debug
{
extern bool active_sequence;
extern bool render_wireframe;
} // namespace debug

namespace debug
{
void init(void);
void layout(void);
} // namespace debug

#endif /* CLIENT_DEBUG_HH */
