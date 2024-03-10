// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef CLIENT_IMAGE_HH
#define CLIENT_IMAGE_HH
#include <string>

struct Image final {
    int width {};
    int height {};
    void *data {};

public:
    static bool load_gray(Image &image, const std::string &path, bool flip);
    static bool load_rgba(Image &image, const std::string &path, bool flip);
    static void unload(Image &image);
};

#endif /* CLIENT_IMAGE_HH */
