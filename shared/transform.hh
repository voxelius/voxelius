// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef EF33F6A4_03E8_4759_8E02_43BC2E4401AD
#define EF33F6A4_03E8_4759_8E02_43BC2E4401AD
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct TransformComponent final {
    glm::vec3 position {};
    glm::quat rotation {};
};

#endif /* EF33F6A4_03E8_4759_8E02_43BC2E4401AD */
