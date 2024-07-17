// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <glm/vec3.hpp>

struct TransformComponent final {
    glm::dvec3 angles {};
    glm::dvec3 position {};
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
