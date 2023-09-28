// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef F86D32A9_E203_4B86_B07B_280C5F59669C
#define F86D32A9_E203_4B86_B07B_280C5F59669C
#include <client/glxx/texture.hh>
#include <shared/vfs.hh>

struct AtlasTexture final {
    uint32_t texture {};
};

namespace atlas
{
void create(int width, int height, size_t count);
void generate_mipmap();
void destroy();
const AtlasTexture *load(const vfs::path_t &path);
const AtlasTexture *find(const vfs::path_t &path);
const AtlasTexture *find(const uint32_t texture);
const glxx::Texture2DArray &get();
} // namespace atlas

#endif/* F86D32A9_E203_4B86_B07B_280C5F59669C */
