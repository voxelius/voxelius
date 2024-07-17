// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_VELOCITY_HH
#define SHARED_ENTITY_VELOCITY_HH
#include <glm/vec3.hpp>

struct VelocityComponent final {
    glm::dvec3 angular {};
    glm::dvec3 linear {};
};

#endif /* SHARED_ENTITY_VELOCITY_HH */
