// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_HEAD_HH
#define SHARED_ENTITY_HEAD_HH
#include <shared/angle3D.hh>

struct HeadComponent final {
    Vector3D offset {};
    Angle3D angles {};
};

#endif /* SHARED_ENTITY_HEAD_HH */
