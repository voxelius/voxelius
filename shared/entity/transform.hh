// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <shared/angle3D.hh>
#include <shared/coord.hh>

struct TransformComponent final {
    WorldPos position {};
    Angle3D angles {};
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
