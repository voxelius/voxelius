// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VIEW_HH
#define CLIENT_VIEW_HH
#include <shared/coord.hh>

namespace view
{
extern float vertical_fov;
extern unsigned int max_distance;
} // namespace view

namespace view
{
extern glm::fvec3 angles;
extern glm::fvec3 direction;
extern glm::fmat4x4 matrix;
extern EntityPos position;
} // namespace view

namespace view
{
void init(void);
void update(void);
} // namespace view

#endif /* CLIENT_VIEW_HH */
