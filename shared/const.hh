// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONST_HH
#define SHARED_CONST_HH
#include <shared/cxmath.hh>

constexpr static const size_t CHUNK_SIZE = 16;
constexpr static const size_t CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_SIZE_LOG2 = cxmath::log2(CHUNK_SIZE);

#endif /* SHARED_CONST_HH */
