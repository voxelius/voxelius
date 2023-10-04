// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_LABEL_HH
#define CLIENT_UI_LABEL_HH
#include <client/gl_texture.hh>
#include <shared/types.hh>
#include <string>

namespace ui
{
class Label final {
public:
    void set(const std::string &text);
    void set(const std::wstring &text);
    void destroy();

    const gl::Texture2D &get() const;
    const size_t get_size() const;

private:
    gl::Texture2D texture {};
    size_t text_size {};
};
} // namespace ui

#endif/* CLIENT_UI_LABEL_HH */
