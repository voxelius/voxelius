// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_PROPERTIES_HH
#define SHARED_PROPERTIES_HH
#include <shared/vfs.hh>
#include <stdint.h>
#include <unordered_map>

class Properties final {
public:
    using vmap_type = std::unordered_map<std::string, std::string>;
    using smap_type = std::unordered_map<std::string, vmap_type>;

public:
    bool read(const vfs::path_t &path);
    bool write(const vfs::path_t &path) const;

    bool get_bool(const std::string &section, const std::string &name, bool stub = false) const;
    intmax_t get_int(const std::string &section, const std::string &name, intmax_t stub = 0) const;
    double get_double(const std::string &section, const std::string &name, double stub = 0.0) const;
    std::string get_string(const std::string &section, const std::string &name, const std::string &stub = std::string{}) const;

    void set_bool(const std::string &section, const std::string &name, bool value);
    void set_int(const std::string &section, const std::string &name, intmax_t value);
    void set_double(const std::string &section, const std::string &name, double value);
    void set_string(const std::string &section, const std::string &name, const std::string &value);

private:
    smap_type sections {};
};

#endif /* SHARED_PROPERTIES_HH */

