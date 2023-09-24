// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef C40B2304_9C5D_4747_83E0_7383EF7EC8B5
#define C40B2304_9C5D_4747_83E0_7383EF7EC8B5
#include <client/glxx/texture.hh>
#include <shared/types.hh>
#include <string>

struct AtlasTexture final {
    uint32_t texture {};
    vec2f_t max_uv {};
};

namespace atlas
{
void create(int width, int height, size_t count);
void destroy();
void generate_mipmap();
const AtlasTexture *emplace(const std::string &path);
const AtlasTexture *find(const std::string &path);
const AtlasTexture *find(const uint32_t texture);
const glxx::Texture2DArray &get();
} // namespace atlas

#endif/* C40B2304_9C5D_4747_83E0_7383EF7EC8B5 */
