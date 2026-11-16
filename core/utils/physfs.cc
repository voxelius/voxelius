#include "core/pch.hh"

#include "core/utils/physfs.hh"

bool utils::read_file(std::string_view path, std::string& buffer)
{
    auto path_unfucked = std::string(path);
    auto file = PHYSFS_openRead(path_unfucked.c_str());

    if(file == nullptr) {
        msg::log_warning("{}: {}", path_unfucked, physfs_error());
        return false;
    }

    // std::string is already null-terminated via .c_str()/.data() (C++11+),
    // so no extra byte is needed here - unlike the vector<byte> overload
    // below, an extra byte here would end up inside source.size() and get
    // fed to consumers (eg. luaL_loadbuffer) as a bogus trailing NUL
    buffer.resize(PHYSFS_fileLength(file), static_cast<char>(0x00));

    PHYSFS_readBytes(file, buffer.data(), buffer.size());
    PHYSFS_close(file);

    return true;
}

bool utils::read_file(std::string_view path, std::vector<std::byte>& buffer)
{
    auto path_unfucked = std::string(path);
    auto file = PHYSFS_openRead(path_unfucked.c_str());

    if(file == nullptr) {
        msg::log_warning("{}: {}", path_unfucked, physfs_error());
        return false;
    }

    buffer.resize(1 + PHYSFS_fileLength(file), static_cast<std::byte>(0x00));

    PHYSFS_readBytes(file, buffer.data(), buffer.size());
    PHYSFS_close(file);

    return true;
}

bool utils::read_file(std::string_view path, std::istringstream& stream)
{
    std::string buffer;

    if(read_file(path, buffer)) {
        stream.str(buffer);
        return true;
    }

    return false;
}

bool utils::write_file(std::string_view path, std::string_view buffer, bool append)
{
    auto path_unfucked = std::string(path);
    PHYSFS_File* file = nullptr;

    if(append) {
        file = PHYSFS_openAppend(path_unfucked.c_str());
    }
    else {
        file = PHYSFS_openWrite(path_unfucked.c_str());
    }

    if(file == nullptr) {
        msg::log_warning("{}: {}", path_unfucked, physfs_error());
        return false;
    }

    PHYSFS_writeBytes(file, buffer.data(), buffer.size());
    PHYSFS_close(file);

    return true;
}

bool utils::write_file(std::string_view path, std::span<const std::byte> buffer, bool append)
{
    auto path_unfucked = std::string(path);
    PHYSFS_File* file = nullptr;

    if(append) {
        file = PHYSFS_openAppend(path_unfucked.c_str());
    }
    else {
        file = PHYSFS_openWrite(path_unfucked.c_str());
    }

    if(file == nullptr) {
        msg::log_warning("{}: {}", path_unfucked, physfs_error());
        return false;
    }

    PHYSFS_writeBytes(file, buffer.data(), buffer.size());
    PHYSFS_close(file);

    return true;
}

std::string_view utils::physfs_error(void)
{
    return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}
