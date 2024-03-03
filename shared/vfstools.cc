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
#include <shared/vfstools.hh>

bool vfstools::readline(PHYSFS_File *file, std::string &line)
{
    if(file && !PHYSFS_eof(file)) {
        line.clear();

        unsigned char character;
        while(PHYSFS_readBytes(file, &character, 1) == 1) {
            if(character == '\n')
                break;
            line.push_back(character);
        }

        return true;
    }

    return false;
}

bool vfstools::read(const std::string &path, std::string &data)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        data.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, data.data(), data.size());
        PHYSFS_close(file);
        return true;
    }

    return false;
}

bool vfstools::read(const std::string &path, std::vector<uint8_t> &data)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        data.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, data.data(), data.size());
        PHYSFS_close(file);
        return true;
    }

    return false;
}

bool vfstools::write(const std::string &path, const std::string &data)
{
    if(PHYSFS_File *file = PHYSFS_openWrite(path.c_str())) {
        PHYSFS_writeBytes(file, data.data(), data.size());
        PHYSFS_close(file);
        return true;
    }

    return false;
}

bool vfstools::write(const std::string &path, const std::vector<uint8_t> &data)
{
    if(PHYSFS_File *file = PHYSFS_openWrite(path.c_str())) {
        PHYSFS_writeBytes(file, data.data(), data.size());
        PHYSFS_close(file);
        return true;
    }

    return false;
}
