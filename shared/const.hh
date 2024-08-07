// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CONST_HH
#define SHARED_CONST_HH
#include <shared/math/constexpr.hh>

constexpr static std::size_t CHUNK_SIZE = 16;
constexpr static std::size_t CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_SIZE_LOG2 = cxpr::log2(CHUNK_SIZE);

#endif /* SHARED_CONST_HH */
