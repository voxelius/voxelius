// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <shared/coord.hh>

struct TransformComponent final {
    EntityPos position {};
    glm::fvec3 angles {};
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
