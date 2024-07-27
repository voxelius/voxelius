// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_DEBUG_KEYS_HH
#define CLIENT_DEBUG_KEYS_HH

namespace debug_keys
{
// The value is true whenever the debug
// toggles manager awaits for a sequenced key
// to be pressed. During this no input should
// be processed by any other gameplay system
extern bool is_sequence_await;
} // namespace debug_keys

namespace debug_keys
{
extern bool draw_bounding_boxes;
extern bool draw_chunk_lines;
extern bool draw_raycasts;
extern bool render_wireframe;
} // namespace debug_keys

namespace debug_keys
{
void init(void);
} // namespace debug_keys

#endif /* CLIENT_DEBUG_KEYS_HH */
