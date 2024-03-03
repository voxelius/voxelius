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
#include <shared/vfs.hh>

bool vfs::init(const std::string &argv_0)
{
    return PHYSFS_init(argv_0.c_str());
}

bool vfs::deinit()
{
    return PHYSFS_deinit();
}

bool vfs::set_write_path(const vfs::path_t &path)
{
    // Thanks to PQCraft for pointing out u8string exists
    // FIX: not that big of a thanks if you consider it to be
    // only a C++20 (and maybe GNU C++ as well) feature
    return PHYSFS_setWriteDir(path.string().c_str());
}

bool vfs::create_directories(const vfs::path_t &path)
{
    return PHYSFS_mkdir(path.string().c_str());
}

bool vfs::mount(const vfs::path_t &path, const vfs::path_t &mountpoint, bool append)
{
    // Thanks to PQCraft for pointing out u8string exists
    // FIX: not that big of a thanks if you consider it to be
    // only a C++20 (and maybe GNU C++ as well) feature
    return PHYSFS_mount(path.string().c_str(), mountpoint.string().c_str(), append ? 1 : 0);
}

bool vfs::umount(const vfs::path_t &path)
{
    // Thanks to PQCraft for pointing out u8string exists
    // FIX: not that big of a thanks if you consider it to be
    // only a C++20 (and maybe GNU C++ as well) feature
    return PHYSFS_unmount(path.string().c_str());
}

bool vfs::exists(const vfs::path_t &path)
{
    return PHYSFS_exists(path.string().c_str()) ? true : false;
}

bool vfs::is_directory(const vfs::path_t &path)
{
    PHYSFS_Stat s = {};
    const std::string vstr = path.string();
    if(PHYSFS_stat(vstr.c_str(), &s))
        return s.filetype == PHYSFS_FILETYPE_DIRECTORY;
    return false;
}

bool vfs::is_symlink(const vfs::path_t &path)
{
    PHYSFS_Stat s = {};
    const std::string vstr = path.string();
    if(PHYSFS_stat(vstr.c_str(), &s))
        return s.filetype == PHYSFS_FILETYPE_SYMLINK;
    return false;
}

void vfs::close(vfs::file_t *file)
{
    PHYSFS_close(file);
}

vfs::file_t *vfs::open(const vfs::path_t &path, vfs::open_mode_t mode)
{
    if(mode & vfs::OPEN_RD)
        return PHYSFS_openRead(path.string().c_str());
    if((mode & vfs::OPEN_WR) && (mode & vfs::OPEN_AP))
        return PHYSFS_openAppend(path.string().c_str());
    if((mode & vfs::OPEN_WR))
        return PHYSFS_openWrite(path.string().c_str());
    return nullptr;
}

size_t vfs::get_length(vfs::file_t *file)
{
    return file ? static_cast<size_t>(PHYSFS_fileLength(file)) : 0;
}

size_t vfs::get_position(vfs::file_t *file)
{
    return file ? static_cast<size_t>(PHYSFS_tell(file)) : vfs::NPOS;
}

size_t vfs::read(vfs::file_t *file, void *buffer, size_t size)
{
    if(file)
        return static_cast<size_t>(PHYSFS_readBytes(file, buffer, static_cast<PHYSFS_uint64>(size)));
    return 0;
}

size_t vfs::write(vfs::file_t *file, const void *buffer, size_t size)
{
    if(file)
        return static_cast<size_t>(PHYSFS_writeBytes(file, buffer, static_cast<PHYSFS_uint64>(size)));
    return 0;
}

bool vfs::read_line(file_t *file, std::string &out)
{
    if(file && !PHYSFS_eof(file)) {
        out.clear();
        char tmp[2] = {0};
        while(PHYSFS_readBytes(file, tmp, 1) == 1) {
            tmp[1] = 0x00; // make sure it's terminated.
            if(tmp[0] == '\n')
                break;
            out.append(tmp);
        }

        return true;
    }

    return false;
}

bool vfs::set_position(vfs::file_t *file, size_t position)
{
    return PHYSFS_seek(file, static_cast<PHYSFS_uint64>(position));
}

const vfs::path_t vfs::get_write_path()
{
    return vfs::path_t(PHYSFS_getWriteDir());
}

const vfs::path_t vfs::get_root_path()
{
    return vfs::path_t("/");
}

const std::string vfs::get_error()
{
    return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

bool vfs::read_bytes(const vfs::path_t &path, std::vector<uint8_t> &out)
{
    vfs::file_t *file = vfs::open(path, vfs::OPEN_RD);
    if(file) {
        out.resize(vfs::get_length(file));
        vfs::read(file, out.data(), out.size());
        vfs::close(file);
        return true;
    }

    return false;
}

bool vfs::read_string(const vfs::path_t &path, std::string &out)
{
    vfs::file_t *file = vfs::open(path, vfs::OPEN_RD);
    if(file) {
        out.resize(vfs::get_length(file));
        vfs::read(file, out.data(), out.size());
        vfs::close(file);
        return true;
    }

    return false;
}

bool vfs::write_bytes(const vfs::path_t &path, const std::vector<uint8_t> &in)
{
    vfs::file_t *file = vfs::open(path, vfs::OPEN_WR);
    if(file) {
        vfs::write(file, in.data(), in.size());
        vfs::close(file);
        return true;
    }

    return false;
}

bool vfs::write_string(const vfs::path_t &path, const std::string &in)
{
    vfs::file_t *file = vfs::open(path, vfs::OPEN_WR);
    if(file) {
        vfs::write(file, in.data(), in.size());
        vfs::write(file, "\n", 1);
        vfs::close(file);
        return true;
    }

    return false;
}
