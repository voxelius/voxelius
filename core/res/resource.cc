#include "core/pch.hh"

#include "core/res/resource.hh"

struct Loader final {
    res::load_func load_fn;
    res::free_func free_fn;

    vx::hash_map<std::string, res::handle<void>> resources;

    std::vector<res::handle<void>> cache;

    std::string classname;
};

static vx::hash_map<std::type_index, std::unique_ptr<Loader>> s_loaders;

void res::detail::register_loader(const std::type_info& type, load_func load_fn, free_func free_fn)
{
    assert(load_fn);
    assert(free_fn);

    auto loader = std::make_unique<Loader>();
    loader->classname = type.name();
    loader->load_fn = load_fn;
    loader->free_fn = free_fn;

    std::type_index type_index(type);

    assert(0 == s_loaders.count(type_index));

    msg::log_debug("registering new loader for <{}>", loader->classname);

    s_loaders.insert_or_assign(type_index, std::move(loader));
}

res::handle<void> res::detail::load_resource(const std::type_info& type, std::string_view path, std::uint32_t flags)
{
    auto full_path = std::string(path);
    auto type_index = std::type_index(type);

    auto loader = s_loaders.find(type_index);

    if(loader == s_loaders.cend()) {
        msg::log_warning("no loader present for <{}>", type.name());
        return nullptr;
    }

    auto found = loader->second->resources.find(full_path);

    if(found == loader->second->resources.cend()) {
        auto raw = loader->second->load_fn(full_path.c_str(), flags);

        if(raw == nullptr) {
            msg::log_warning("{}<{}>: load failed", full_path, type.name());
            return nullptr;
        }

        auto no_delete = [](const void*) {
            // empty
        };

        handle<void> resource(raw, no_delete);

        auto loaded = loader->second->resources.insert_or_assign(full_path, std::move(resource));

        if(flags & RESFLAG_CACHE) {
            loader->second->cache.push_back(loaded.first->second);
        }

        return loaded.first->second;
    }

    return found->second;
}

res::handle<void> res::detail::find_resource(const std::type_info& type, std::string_view path)
{
    auto full_path = std::string(path);
    auto type_index = std::type_index(type);

    auto loader = s_loaders.find(type_index);

    if(loader == s_loaders.cend()) {
        msg::log_warning("no loader present for <{}>", type.name());
        return nullptr;
    }

    auto found = loader->second->resources.find(full_path);

    if(found == loader->second->resources.cend()) {
        return nullptr;
    }

    return found->second;
}

void res::soft_purge(bool include_cached)
{
    for(const auto& it : s_loaders) {
        auto& loader = it.second;

        if(include_cached) {
            // Normally soft_purge is called after every frame is rendered,
            // but it also may be called whenever we disconnect, so certain
            // cached resources might need to be freed; calling hard_purge
            // is also not an option specifically because it doesn't care
            // about use_count and very unpleasant things (UAF) might happen
            loader->cache.clear();
        }

        auto iter = loader->resources.begin();

        while(iter != loader->resources.end()) {
            if(iter->second.use_count() <= 1) {
                msg::log_debug("releasing {}<{}>", iter->first, loader->classname);

                loader->free_fn(iter->second.get());

                iter = loader->resources.erase(iter);
            }
            else {
                iter = std::next(iter);
            }
        }
    }
}

void res::hard_purge(void)
{
    for(auto& outer : s_loaders) {
        auto& loader = outer.second;

        loader->cache.clear();

        for(auto& inner : loader->resources) {
            auto& path = inner.first;
            auto& handle = inner.second;

            if(handle.use_count() > 1) {
                msg::log_warning("zombie resource: {}<{}> use_count={}", path, loader->classname, handle.use_count());
            }

            msg::log_debug("releasing {}<{}>", path, loader->classname);

            loader->free_fn(handle.get());
        }

        loader->resources.clear();
    }
}
