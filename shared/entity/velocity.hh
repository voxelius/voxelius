// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_VELOCITY_HH
#define SHARED_ENTITY_VELOCITY_HH
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

struct VelocityComponent final {
    glm::fvec3 angular {};
    glm::fvec3 linear {};
};

#endif /* SHARED_ENTITY_VELOCITY_HH */
