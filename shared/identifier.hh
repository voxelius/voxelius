// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B4F0F918_8031_4CF1_9D05_740B5BCB413F
#define B4F0F918_8031_4CF1_9D05_740B5BCB413F
#include <functional>
#include <string>

// A namespaced identifier.
// Works pretty much the same like one
// would expect it to work in Minecraft:
// 1. For textures:
//  voxelius:voxels/stone.png -> /voxelius/textures/voxel/stone.png
// 2. For materials:
//  voxelius:stone -> stone at /voxelius/materials.json
// 3. For block definitons:
//  voxelius:stone -> /voxelius/voxels/stone.json
class Identifier final {
public:
    constexpr static const char *DEFAULT_NAMESPACE = "voxelius";

public:
    Identifier() = default;
    Identifier(const std::string &full);
    Identifier(const std::string &space, const std::string &value);

    const std::string get() const;
    const std::string &get_namespace() const;
    const std::string &get_value() const;
    void set_namespace(const std::string &space);
    void set_value(const std::string &value);
    void set(const std::string &full);

    // Filename in form of /<namespace>/<subdirectory>/<value>.<ext>
    // Used for textures, sounds and whatever the fuck the game wants.
    std::string get_file_path() const;
    std::string get_file_path(const std::string &subdirectory) const;
    std::string get_file_path_ext(const std::string &ext) const;
    std::string get_file_path_ext(const std::string &subdirectory, const std::string &ext) const;

private:
    std::string space {DEFAULT_NAMESPACE};
    std::string value {};
};

template<>
struct std::hash<Identifier> final {
    size_t operator()(const Identifier &identifier) const
    {
        std::hash<std::string> hasher {};
        return hasher(identifier.get_namespace()) + hasher(identifier.get_value());
    }
};

static inline bool operator==(const Identifier &ida, const Identifier &idb)
{
    if(ida.get_namespace().compare(idb.get_namespace()))
        return false;
    if(ida.get_value().compare(idb.get_value()))
        return false;
    return true;
}

#endif/* B4F0F918_8031_4CF1_9D05_740B5BCB413F */
