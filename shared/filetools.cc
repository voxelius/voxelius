// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/filetools.hh>

bool filetools::read(const std::string &path, std::string &out)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        out.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, out.data(), out.size());
        PHYSFS_close(file);
        return true;
    }

    return true;
}

bool filetools::read(const std::string &path, std::vector<uint8_t> &out)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        out.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, out.data(), out.size());
        PHYSFS_close(file);
        return true;
    }

    return true;
}
