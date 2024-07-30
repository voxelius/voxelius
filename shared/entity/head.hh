// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_HEAD_HH
#define SHARED_ENTITY_HEAD_HH
#include <shared/math/vec3angles.hh>

struct HeadComponent final {
    Vec3angles angles {};
    Vec3f offset {};
};

#endif /* SHARED_ENTITY_HEAD_HH */
