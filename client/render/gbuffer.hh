// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_RENDER_GBUFFER_HH
#define CLIENT_RENDER_GBUFFER_HH
#include <client/render/gl/framebuffer.hh>

class GBuffer final {
public:
    void create(int width, int height);
    void destroy();

    const gl::Texture2D &get_albedo_attachment() const;
    const gl::Texture2D &get_normal_attachment() const;

    const gl::Renderbuffer &get_renderbuffer() const;
    const gl::Framebuffer &get_framebuffer() const;

private:
    gl::Texture2D albedo_attachment;
    gl::Texture2D normal_attachment;
    gl::Renderbuffer renderbuffer;
    gl::Framebuffer framebuffer;
};

#endif/* CLIENT_RENDER_GBUFFER_HH */
