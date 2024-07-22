// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

struct TransformComponent final {
    glm::fvec3 position {};
    glm::fvec3 angles {};
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
