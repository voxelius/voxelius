// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_MIXIN_HH
#define SHARED_MIXIN_HH

class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable &rhs) = delete;
    NonCopyable &operator=(const NonCopyable &rhs) = delete;
};

class NonMovable {
    NonMovable() = default;
    NonMovable(NonMovable &&rhs) = delete;
    NonMovable &operator=(NonMovable &&rhs) = delete;
};

#endif /* SHARED_MIXIN_HH */
