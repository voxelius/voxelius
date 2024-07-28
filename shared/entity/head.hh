// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_HEAD_HH
#define SHARED_ENTITY_HEAD_HH
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct HeadComponent final {
    glm::fvec3 angles {};
    glm::fvec3 offset {};
};

#endif /* SHARED_ENTITY_HEAD_HH */
