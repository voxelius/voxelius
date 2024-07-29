// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_DEBUG_DEBUG_TOGGLES_HH
#define CLIENT_DEBUG_DEBUG_TOGGLES_HH

namespace debug_toggles
{
// The value is true whenever the debug
// toggles manager awaits for a sequenced key
// to be pressed. During this no input should
// be processed by any other gameplay system
extern bool is_sequence_await;
} // namespace debug_toggles

namespace debug_toggles
{
extern bool draw_bounding_boxes;
extern bool draw_chunk_lines;
extern bool draw_raycasts;
extern bool render_wireframe;
} // namespace debug_toggles

namespace debug_toggles
{
void init(void);
} // namespace debug_toggles

#endif /* CLIENT_DEBUG_DEBUG_TOGGLES_HH */
