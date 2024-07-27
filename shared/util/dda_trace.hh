// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_DDA_TRACE_HH
#define SHARED_UTIL_DDA_TRACE_HH
#include <shared/trace.hh>

namespace util
{
bool dda_trace(const EntityPos &start, const glm::fvec3 &direction, float max_distance);
bool dda_trace(const EntityPos &start, const glm::fvec3 &direction, float max_distance, Trace &trace);
} // namespace util

#endif /* SHARED_UTIL_DDA_TRACE_HH */
