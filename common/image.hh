// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <string>

class Image final {
public:
    int width {};
    int height {};
    void *pixels {};

public:
    static bool load_gray(Image &image, const std::string &path, bool flip);
    static bool load_rgba(Image &image, const std::string &path, bool flip);
    static void unload(Image &image);
};
