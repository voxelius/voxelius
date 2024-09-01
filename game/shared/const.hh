// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/constexpr.hh>

constexpr static std::size_t CHUNK_SIZE = 32;
constexpr static std::size_t CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_SIZE_LOG2 = cxpr::log2(CHUNK_SIZE);
