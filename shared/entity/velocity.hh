// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_VELOCITY_HH
#define SHARED_ENTITY_VELOCITY_HH
#include <shared/angle3D.hh>

struct VelocityComponent final {
    Vector3D linear {};
    Angle3D angular {};
};

#endif /* SHARED_ENTITY_VELOCITY_HH */
