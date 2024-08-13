// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_VELOCITY_HH
#define SHARED_ENTITY_VELOCITY_HH
#include <shared/math/vec3angles.hh>

class VelocityComponent final {
public:
    Vec3angles angular {};
    Vec3f linear {};

public:
    // Updates entities TransformComponent values
    // according to velocities multiplied by frametime.
    // This system was previously called inertial
    static void update(void);
};

#endif /* SHARED_ENTITY_VELOCITY_HH */
