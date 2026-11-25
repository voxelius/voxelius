#ifndef BE3E6E7A_D7CE_4392_BEBB_1A7AB35C0124
#define BE3E6E7A_D7CE_4392_BEBB_1A7AB35C0124

#include "core/identifier.hh"

constexpr static std::uint32_t RESFLAG_CACHE = 1 << 0;
constexpr static std::uint32_t RESFLAG_CUSTOM = 0xFFFFFF00;

namespace res
{
template<typename T>
using handle = std::shared_ptr<const T>;
} // namespace res

namespace res
{
using load_func = const void* (*)(const char* path, std::uint32_t flags);
using free_func = void (*)(const void* resource);
} // namespace res

namespace res::detail
{
void register_loader(const std::type_info& type, load_func load_fn, free_func free_fn);
handle<void> load_resource(const std::type_info& type, std::string_view path, std::uint32_t flags);
handle<void> find_resource(const std::type_info& type, std::string_view path);
} // namespace res::detail

namespace res
{
template<typename T>
void register_loader(load_func load_fn, free_func free_fn);
} // namespace res

namespace res
{
template<typename T>
handle<T> load_by_path(std::string_view path, std::uint32_t flags = 0);
template<typename T>
handle<T> find_by_path(std::string_view path);
} // namespace res

namespace res
{
template<typename T>
handle<T> load(const Identifier& id, std::uint32_t flags = 0);
template<typename T>
handle<T> load(const Identifier& id, std::string_view subdirectory, std::uint32_t flags = 0);
template<typename T>
handle<T> load(const Identifier& id, std::string_view subdirectory, std::string_view extension, std::uint32_t flags = 0);
} // namespace res

namespace res
{
template<typename T>
handle<T> find(const Identifier& id);
template<typename T>
handle<T> find(const Identifier& id, std::string_view subdirectory);
template<typename T>
handle<T> find(const Identifier& id, std::string_view subdirectory, std::string_view extension);
} // namespace res

namespace res
{
void soft_purge(bool include_cached = false);
void hard_purge(void);
} // namespace res

template<typename T>
void res::register_loader(load_func load_fn, free_func free_fn)
{
    res::detail::register_loader(typeid(T), load_fn, free_fn);
}

template<typename T>
res::handle<T> res::load_by_path(std::string_view path, std::uint32_t flags)
{
    auto result = res::detail::load_resource(typeid(T), path, flags);
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::find_by_path(std::string_view path)
{
    auto result = res::detail::find_resource(typeid(T), path);
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::load(const Identifier& id, std::uint32_t flags)
{
    auto path = id.as_file_path({});
    auto result = res::detail::load_resource(typeid(T), path, flags);
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::load(const Identifier& id, std::string_view subdirectory, std::uint32_t flags)
{
    auto path = id.as_file_path(subdirectory, {});
    auto result = res::detail::load_resource(typeid(T), path, flags);
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::load(const Identifier& id, std::string_view subdirectory, std::string_view extension, std::uint32_t flags)
{
    auto path = id.as_file_path(subdirectory, extension);
    auto result = res::detail::load_resource(typeid(T), path, flags);
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::find(const Identifier& id)
{
    auto result = res::detail::find_resource(typeid(T), id.as_file_path({}));
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::find(const Identifier& id, std::string_view subdirectory)
{
    auto result = res::detail::find_resource(typeid(T), id.as_file_path(subdirectory, {}));
    return std::reinterpret_pointer_cast<const T>(result);
}

template<typename T>
res::handle<T> res::find(const Identifier& id, std::string_view subdirectory, std::string_view extension)
{
    auto result = res::detail::find_resource(typeid(T), id.as_file_path(subdirectory, extension));
    return std::reinterpret_pointer_cast<const T>(result);
}

#endif /* BE3E6E7A_D7CE_4392_BEBB_1A7AB35C0124 */
