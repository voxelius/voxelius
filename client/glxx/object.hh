// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLXX_OBJECT_HH
#define CLIENT_GLXX_OBJECT_HH
#include <glad/gl.h>
#include <shared/mixin.hh>
#include <stddef.h>
#include <stdint.h>
#include <utility>

namespace glxx
{
template<typename T>
class Object : public NonCopyable {
public:
    Object() = default;
    virtual ~Object();
    void destroy();
    constexpr bool valid() const;
    constexpr uint32_t get() const;

protected:
    uint32_t handle {0};
};
} // namespace glxx

template<typename T>
inline glxx::Object<T>::~Object()
{
    destroy();
}

template<typename T>
inline void glxx::Object<T>::destroy()
{
    static_cast<T *>(this)->destroy();
}

template<typename T>
inline constexpr bool glxx::Object<T>::valid() const
{
    return handle != 0;
}

template<typename T>
inline constexpr uint32_t glxx::Object<T>::get() const
{
    return handle;
}

#endif /* CLIENT_GLXX_OBJECT_HH */
