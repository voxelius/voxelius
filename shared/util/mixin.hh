// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_MIXIN_HH
#define SHARED_UTIL_MIXIN_HH

namespace util
{
class NonCopyable;
class NonMovable;
} // namespace util

class util::NonCopyable {
public:
    NonCopyable(void) = default;
    NonCopyable(const NonCopyable &other) = delete;
    NonCopyable &operator=(const NonCopyable &other) = delete;
};

class util::NonMovable {
public:
    NonMovable(void) = default;
    NonMovable(NonMovable &&other) = delete;
    NonMovable &operator=(NonMovable &&other) = delete;
};

#endif /* SHARED_UTIL_MIXIN_HH */
