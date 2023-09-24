// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/filetools.hh>

bool util::read(const std::string &path, std::string &out)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        out.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, out.data(), out.size());
        PHYSFS_close(file);
        return true;
    }

    return true;
}

bool util::read(const std::string &path, std::vector<uint8_t> &out)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        out.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, out.data(), out.size());
        PHYSFS_close(file);
        return true;
    }

    return true;
}

bool util::write(const std::string &path, const std::string &in)
{
    if(PHYSFS_File *file = PHYSFS_openWrite(path.c_str())) {
        PHYSFS_writeBytes(file, in.data(), in.size());
        PHYSFS_close(file);
        return true;
    }

    return true;
}

bool util::write(const std::string &path, const std::vector<uint8_t> &in)
{
    if(PHYSFS_File *file = PHYSFS_openWrite(path.c_str())) {
        PHYSFS_writeBytes(file, in.data(), in.size());
        PHYSFS_close(file);
        return true;
    }

    return true;
}

bool util::readline(PHYSFS_File *file, std::string &line)
{
    if(file && !PHYSFS_eof(file)) {
        line.clear();
        char tmp[2] = {0};
        while(PHYSFS_readBytes(file, tmp, 1) == 1) {
            tmp[1] = 0x00; // make sure it's terminated.
            if(tmp[0] == '\n')
                break;
            line.append(tmp);
        }

        return true;
    }

    return false;
}
