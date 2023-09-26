// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef A52A1506_280E_435B_A63C_DF288D91207F
#define A52A1506_280E_435B_A63C_DF288D91207F
#include <filesystem>
#include <limits>
#include <physfs.h>
#include <vector>

namespace vfs
{
using file_t = PHYSFS_File;
using open_mode_t = unsigned short;
using path_t = std::filesystem::path;

struct path_hasher_t final {
    inline const size_t operator()(const vfs::path_t &path) const
    {
        return std::filesystem::hash_value(path);
    }
};

constexpr static const open_mode_t OPEN_RD = open_mode_t(1 << 0);
constexpr static const open_mode_t OPEN_WR = open_mode_t(1 << 1);
constexpr static const open_mode_t OPEN_AP = open_mode_t(1 << 2);

constexpr static const size_t NPOS = std::numeric_limits<size_t>::max();

bool init(const std::string &argv_0);
bool deinit();

bool set_write_path(const path_t &path);
bool create_directories(const path_t &path);
bool mount(const path_t &path, const path_t &mountpoint, bool append);
bool umount(const path_t &path);

bool exists(const path_t &path);
bool is_directory(const path_t &path);
bool is_symlink(const path_t &path);

void close(file_t *file);
file_t *open(const path_t &path, open_mode_t mode);
size_t get_length(file_t *file);
size_t get_position(file_t *file);
size_t read(file_t *file, void *buffer, size_t size);
size_t write(file_t *file, const void *buffer, size_t size);
bool read_line(file_t *file, std::string &out);
bool set_position(file_t *file, size_t position);

const path_t get_write_path();
const path_t get_root_path();
const std::string get_error();

bool read_bytes(const path_t &path, std::vector<uint8_t> &out);
bool read_string(const path_t &path, std::string &out);
bool write_bytes(const path_t &path, const std::vector<uint8_t> &in);
bool write_string(const path_t &path, const std::string &in);
} // namespace vfs

#endif/* A52A1506_280E_435B_A63C_DF288D91207F */
