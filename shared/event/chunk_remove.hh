// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_EVENT_CHUNK_REMOVE_HH
#define SHARED_EVENT_CHUNK_REMOVE_HH
#include <shared/chunks.hh>

struct ChunkRemoveEvent final {
    Chunk *chunk {nullptr};
    chunk_pos_t cpos {};
};

#endif /* SHARED_EVENT_CHUNK_REMOVE_HH */
