// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_HEAD_HH
#define SHARED_ENTITY_HEAD_HH
#include <shared/math/euler_angles.hh>

struct HeadComponent final {
    EulerAngles angles {};
    Vector3D offset {};
};

#endif /* SHARED_ENTITY_HEAD_HH */
