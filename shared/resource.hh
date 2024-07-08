// SPDX-License-Identifier: Zlib
#ifndef SHARED_RESOURCE_HH
#define SHARED_RESOURCE_HH
#include <string>

struct RefCounted {
    // Whenever the resource instance
    // is released, this value is decremented;
    // When it reaches zero, the resource is freed.
    unsigned short ref_count {};
};

namespace resource
{
template<typename resource_type>
const resource_type *load(const std::string &path);
template<typename resource_type>
const resource_type *find(const std::string &path);
template<typename resource_type>
bool release(const resource_type *object);
template<typename resource_type>
void purge(void);
} // namespace resource

#endif /* SHARED_RESOURCE_HH */
