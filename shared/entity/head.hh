// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_HEAD_HH
#define SHARED_ENTITY_HEAD_HH
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct HeadComponent final {
    glm::dvec2 angles {};
    glm::dvec3 offset {};
};

#endif /* SHARED_ENTITY_HEAD_HH */
