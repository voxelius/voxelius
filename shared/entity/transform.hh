// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <shared/math/vec3angles.hh>
#include <shared/world/coord.hh>

class TransformComponent final {
public:
    Vec3angles angles {};
    WorldCoord position {};

public:
    // Updates TransformComponent values so that
    // the local part of WorldCoord field is always
    // within a single chunk - floating point precision fixes
    static void update(void);
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
