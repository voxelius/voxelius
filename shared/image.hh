// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_IMAGE_HH
#define SHARED_IMAGE_HH
#include <string>

class Image final {
public:
    Image(void) = default;
    Image(const std::string &path, bool flip);
    virtual ~Image(void);

    bool load_gray(const std::string &path, bool flip);
    bool load_rgba(const std::string &path, bool flip);
    void unload(void);

    bool is_valid(void) const;
    int get_width(void) const;
    int get_height(void) const;

    const void *data(void) const;
    void *data(void);

private:
    int width {};
    int height {};
    void *pixels {};
};

#endif /* SHARED_IMAGE_HH */
