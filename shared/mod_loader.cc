#include "shared/pch.hh"

#include "shared/mod_loader.hh"

#include "core/config/map.hh"
#include "core/exception.hh"

#include "shared/constant.hh"
#include "shared/entity/class_registry.hh"
#include "shared/entity/required_class.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/precip_registry.hh"
#include "shared/world/tint_registry.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world_registry.hh"

static std::vector<ModContext> s_mods;
static std::vector<library_opener> s_libraries;

static std::vector<ModInfo> discover_mods(void)
{
    std::vector<ModInfo> result;

    if(auto entries = PHYSFS_enumerateFiles("data")) {
        for(std::size_t i = 0; entries[i]; ++i) {
            PHYSFS_Stat stat {};

            auto entry_path = std::format("data/{}", entries[i]);

            if(!PHYSFS_stat(entry_path.c_str(), &stat)) {
                continue;
            }

            if(stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
                auto modinfo_path = std::format("data/{}/modinfo.conf", entries[i]);

                if(!PHYSFS_exists(modinfo_path.c_str())) {
                    continue;
                }

                config::Map map;

                if(!map.load(modinfo_path)) {
                    msg::log_error("{}: unable to read modinfo.conf", entries[i]);
                    continue;
                }

                ModInfo info {};

                if(!ModInfo::parse(map, info)) {
                    msg::log_error("{}: malformed modinfo.conf", entries[i]);
                    continue;
                }

                result.emplace_back(std::move(info));
            }
        }

        PHYSFS_freeList(entries);
    }

    return result;
}

static std::vector<ModInfo> resolve_load_order(std::vector<ModInfo> mods)
{
    vx::hash_map<std::string, ModInfo*> map;
    vx::hash_map<std::string, ModVersion> versions;

    for(auto& info : mods) {
        map.insert_or_assign(info.name, &info);
        versions.insert_or_assign(info.name, ModVersion(info.version));
    }

    std::unordered_set<std::string> resolved;
    std::unordered_set<std::string> remaining;

    for(const auto& info : mods) {
        remaining.insert(info.name);
    }

    for(const auto& info : mods) {
        for(const auto& conflict : info.conflicts) {
            auto it = versions.find(conflict.first);

            if(it == versions.cend()) {
                continue;
            }

            if(conflict.second == ModVersion {} || it->second <= conflict.second) {
                msg::log_error("{}: conflicts with {}", info.name, conflict.first);
                remaining.erase(info.name);
                break;
            }
        }
    }

    std::vector<ModInfo> result;
    result.reserve(mods.size());

    auto hard_satisfied = [&](const ModInfo* info) {
        return std::all_of(info->hard_depends.cbegin(), info->hard_depends.cend(), [&](const auto& dep) {
            return resolved.contains(dep.first) && versions.at(dep.first) >= dep.second;
        });
    };

    auto soft_satisfied = [&](const ModInfo* info) {
        return std::all_of(info->soft_depends.cbegin(), info->soft_depends.cend(), [&](const auto& dep) {
            return resolved.contains(dep.first) && versions.at(dep.first) >= dep.second;
        });
    };

    while(remaining.size()) {
        std::vector<std::string> ready;
        ready.reserve(remaining.size());

        for(const auto& name : remaining) {
            const auto info = map.at(name);

            if(hard_satisfied(info) && soft_satisfied(info)) {
                ready.push_back(name);
            }
        }

        if(ready.empty()) {
            for(const auto& name : remaining) {
                if(hard_satisfied(map.at(name))) {
                    ready.push_back(name);
                }
            }
        }

        if(ready.empty()) {
            break;
        }

        std::sort(ready.begin(), ready.end());

        for(const auto& name : ready) {
            auto info = map.at(name);
            result.push_back(std::move(*info));
            resolved.insert(name);
            remaining.erase(name);
        }
    }

    std::vector<std::string> unresolved(remaining.begin(), remaining.end());
    std::sort(unresolved.begin(), unresolved.end());

    for(const auto& name : unresolved) {
        auto info = map.at(name);
        auto reported = false;

        for(const auto& dep : info->hard_depends) {
            if(resolved.contains(dep.first)) {
                continue;
            }

            if(versions.contains(dep.first)) {
                msg::log_error("{}: dependency {} was not loaded", name, dep.first);
            }
            else {
                msg::log_error("{}: missing dependency {}", name, dep.first);
            }

            reported = true;
            break;
        }

        if(!reported) {
            msg::log_error("{}: dependency cycle", name);
        }
    }

    return result;
}

void mod_loader::detail::add_library(library_opener opener)
{
    assert(s_mods.empty());

    s_libraries.push_back(std::move(opener));
}

void mod_loader::init(void)
{
    auto mods = resolve_load_order(discover_mods());
    auto libraries = std::span<const library_opener>(s_libraries);

    s_mods.clear();
    s_mods.reserve(mods.size());

    std::unordered_set<std::string> failed;

    for(auto& info : mods) {
        auto blocked = std::any_of(info.hard_depends.cbegin(), info.hard_depends.cend(), [&](const auto& dep) {
            return failed.contains(dep.first);
        });

        if(blocked) {
            msg::log_warning("{}: a hard dependency failed to load", info.name);
            failed.insert(info.name);
            continue;
        }

        auto name = info.name;
        auto& ctx = s_mods.emplace_back(std::move(info));

        if(!ctx.initialize(libraries)) {
            vx::throw_if(name == constant::BUILTIN_NAME_SPACE, "you have met a terrible fate, haven't you?");
            failed.insert(name);
            continue;
        }

        tint_registry::commit(ctx);
        fluid_registry::commit(ctx);
        block_registry::commit(ctx);
        biome_registry::commit(ctx);
        precip_registry::commit(ctx);
        weather_registry::commit(ctx);
        class_registry::commit(ctx);
    }

    fluid_registry::resolve_tints();

    biome_registry::resolve_palettes();
    biome_registry::resolve_features();
    biome_registry::resolve_tint_colors();

    required_class::resolve();
}

void mod_loader::shutdown(void)
{
    s_mods.clear();

    world_registry::purge();
    class_registry::purge();
    weather_registry::purge();
    precip_registry::purge();
    biome_registry::purge();
    block_registry::purge();
    fluid_registry::purge();
    tint_registry::purge();
}

std::span<const ModContext> mod_loader::all(void)
{
    return s_mods;
}

const ModContext* mod_loader::find(std::string_view name_space)
{
    for(const auto& ctx : s_mods) {
        if(ctx.name_space() == name_space) {
            return &ctx;
        }
    }

    return nullptr;
}
