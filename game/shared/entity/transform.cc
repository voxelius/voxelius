// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <game/shared/entity/transform.hh>
#include <game/shared/globals.hh>

void TransformComponent::update(void)
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
