// SPDX-License-Identifier: Zlib
#ifndef SHARED_IMAGE_HH
#define SHARED_IMAGE_HH
#include <glm/vec2.hpp>
#include <shared/resource.hh>

struct Image final : public RefCounted {
    glm::ivec2 size {};
    const void *pixels {};
    std::string path {};
};

namespace detail::image
{
const Image *load(const std::string &path);
const Image *find(const std::string &path);
bool release(const Image *object);
void purge(void);
} // namespace detail::image

template<>
inline const Image *resource::load<Image>(const std::string &path)
{
    return detail::image::load(path);
}

template<>
inline const Image *resource::find<Image>(const std::string &path)
{
    return detail::image::find(path);
}

template<>
inline bool resource::release<Image>(const Image *object)
{
    return detail::image::release(object);
}

template<>
inline void resource::purge<Image>(void)
{
    detail::image::purge();
}

#endif /* SHARED_IMAGE_HH */
