// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_OPTIONS_HH
#define CLIENT_OPTIONS_HH
#include <shared/vfs.hh>

struct OptionsGeneral final {
    double fov {90.0};
};

struct OptionsControls final {
    bool raw_mouse {true};
    double sensitivity {0.25};
};

struct OptionsGraphics final {
    // Nothing yet
};

struct OptionsSound final {
    // Nothing yet
};

struct Options final {
    OptionsGeneral general {};
    OptionsControls controls {};
    OptionsGraphics graphics {};
    OptionsSound sound {};

    static void read(const vfs::path_t &path, Options &out);
    static void write(const vfs::path_t &path, const Options &in);
};

#endif /* CLIENT_OPTIONS_HH */
