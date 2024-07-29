// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_VELOCITY_HH
#define SHARED_ENTITY_VELOCITY_HH
#include <shared/math/euler_angles.hh>

class VelocityComponent final {
public:
    EulerAngles angular {};
    Vector3D linear {};

public:
    // Updates entities TransformComponent values
    // according to velocities multiplied by frametime.
    // This system was previously called inertial
    static void update(float frametime);
};

#endif /* SHARED_ENTITY_VELOCITY_HH */
