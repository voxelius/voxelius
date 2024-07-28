// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <shared/entity/transform.hh>
#include <shared/util/cxmath.hh>
#include <shared/const.hh>
#include <shared/floatfix.hh>
#include <shared/globals.hh>

void floatfix::update(void)
{
    const auto view = globals::registry.view<TransformComponent>();

    for(auto [entity, transform] : view.each()) {
        for(std::size_t i = 0U; i < 3U; ++i) {
            if(transform.position.local[i] >= CHUNK_SIZE) {
                transform.position.local[i] -= CHUNK_SIZE;
                transform.position.chunk[i] += 1;
                continue;
            }

            if(transform.position.local[i] < 0.0f) {
                transform.position.local[i] += CHUNK_SIZE;
                transform.position.chunk[i] -= 1;
                continue;
            }
        }
    }
}
