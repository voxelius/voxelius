// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef ED122468_0EE3_4FC5_9C6A_22F0C364C19C
#define ED122468_0EE3_4FC5_9C6A_22F0C364C19C
#include <client/glxx/framebuffer.hh>

class GBuffer final {
public:
    void create(int width, int height);
    void destroy();

    const glxx::Texture2D &get_albedo_attachment() const;
    const glxx::Texture2D &get_normal_attachment() const;

    const glxx::Renderbuffer &get_renderbuffer() const;
    const glxx::Framebuffer &get_framebuffer() const;

private:
    glxx::Texture2D albedo_attachment;
    glxx::Texture2D normal_attachment;
    glxx::Renderbuffer renderbuffer;
    glxx::Framebuffer framebuffer;
};

#endif/* ED122468_0EE3_4FC5_9C6A_22F0C364C19C */
