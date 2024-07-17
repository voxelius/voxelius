// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <array>
#include <shared/const.hh>
#include <shared/voxel_storage.hh>
#include <spdlog/spdlog.h>
#include <vector>

struct PaletteStorageU8 final {
    std::array<std::uint8_t, CHUNK_VOLUME> indices {};
    std::vector<Voxel> palette {};
};

struct PaletteStorageU16 final {
    std::array<std::uint16_t, CHUNK_VOLUME> indices {};
    std::vector<Voxel> palette {};
};

void voxel_storage::create(VoxelStorage &storage)
{
    storage.type = VOXEL_STORAGE_SINGLE_VOXEL;
    storage.voxels = reinterpret_cast<void *>(NULL_VOXEL);
}

void voxel_storage::clone(VoxelStorage &storage, const VoxelStorage &parent)
{
    if(parent.type == VOXEL_STORAGE_SINGLE_VOXEL) {
        storage.type = VOXEL_STORAGE_SINGLE_VOXEL;
        storage.voxels = parent.voxels;
        return;
    }

    if(parent.type == VOXEL_STORAGE_PALETTE_UINT8) {
        const PaletteStorageU8 *pv = reinterpret_cast<PaletteStorageU8 *>(parent.voxels);

        PaletteStorageU8 *sv = new PaletteStorageU8();
        sv->indices = pv->indices;
        sv->palette = pv->palette;

        storage.type = VOXEL_STORAGE_PALETTE_UINT8;
        storage.voxels = sv;

        return;
    }

    if(parent.type == VOXEL_STORAGE_PALETTE_UINT16) {
        const PaletteStorageU16 *pv = reinterpret_cast<PaletteStorageU16 *>(parent.voxels);

        PaletteStorageU16 *sv = new PaletteStorageU16();
        sv->indices = pv->indices;
        sv->palette = pv->palette;

        storage.type = VOXEL_STORAGE_PALETTE_UINT16;
        storage.voxels = sv;

        return;
    }

    spdlog::error("voxel_storage: potential data corruption");
    spdlog::trace("voxel_storage: >> storage = {}", reinterpret_cast<const void *>(&parent));
}

void voxel_storage::destroy(VoxelStorage &storage)
{
    switch(storage.type) {
        case VOXEL_STORAGE_PALETTE_UINT8:
            delete reinterpret_cast<PaletteStorageU8 *>(storage.voxels);
            break;
        case VOXEL_STORAGE_PALETTE_UINT16:
            delete reinterpret_cast<PaletteStorageU16 *>(storage.voxels);
            break;
    }

    storage.type = VOXEL_STORAGE_SINGLE_VOXEL;
    storage.voxels = nullptr;
}

Voxel voxel_storage::get(const VoxelStorage &storage, const LocalCoord &lv)
{
    return voxel_storage::get(storage, coord::to_index(lv));
}

Voxel voxel_storage::get(const VoxelStorage &storage, std::size_t index)
{
    if(!storage.voxels) {
        // Either not allocated or filled
        // with just NULL_VOXEL voxels
        return NULL_VOXEL;
    }

    if(index >= CHUNK_VOLUME) {
        // Index out of range
        return NULL_VOXEL;
    }

    if(storage.type == VOXEL_STORAGE_SINGLE_VOXEL) {
        // The entire storage is just one voxel
        return static_cast<Voxel>(reinterpret_cast<std::uintptr_t>(storage.voxels));
    }

    if(storage.type == VOXEL_STORAGE_PALETTE_UINT8) {
        const auto *ps = reinterpret_cast<const PaletteStorageU8 *>(storage.voxels);
        const auto entry = ps->indices.at(index);
        return entry ? ps->palette.at(entry - 1) : NULL_VOXEL;
    }

    if(storage.type == VOXEL_STORAGE_PALETTE_UINT16) {
        const auto *ps = reinterpret_cast<const PaletteStorageU16 *>(storage.voxels);
        const auto entry = ps->indices.at(index);
        return entry ? ps->palette.at(entry - 1) : NULL_VOXEL;
    }

    // The code essentially just goes into "what the fuck" mode here
    spdlog::error("voxel_storage: potential data corruption");
    spdlog::trace("voxel_storage: >> storage = {}", reinterpret_cast<const void *>(&storage));
    return NULL_VOXEL;
}

void voxel_storage::set(VoxelStorage &storage, Voxel voxel, const LocalCoord &lv)
{
    voxel_storage::set(storage, voxel, coord::to_index(lv));
}

void voxel_storage::set(VoxelStorage &storage, Voxel voxel, std::size_t index)
{
    if(storage.type == VOXEL_STORAGE_SINGLE_VOXEL) {
        const auto filler = static_cast<Voxel>(reinterpret_cast<std::uintptr_t>(storage.voxels));

        if(voxel == filler) {
            // We don't have to do anything else here
            // since the entire chunk is already filled
            // with the exact same voxel identifier
            return;
        }

        auto *new_ps = new PaletteStorageU8();

        if(filler == NULL_VOXEL) {
            // NULL_VOXEL is a special case for palette
            // storage types; if the index within the swatch
            // is zero, it automatically resolves as NULL_VOXEL
            new_ps->indices.fill(0);
        }
        else {
            new_ps->palette.push_back(filler);
            new_ps->indices.fill(new_ps->palette.size());
        }

        if(voxel == NULL_VOXEL) {
            // The same exact case of NULL_VOXEL being a special
            // case for palettes, except in this case we actually
            // just set the index without touching the palette
            new_ps->indices[index] = 0;
        }
        else {
            new_ps->palette.push_back(voxel);
            new_ps->indices[index] = new_ps->palette.size();
        }

        storage.type = VOXEL_STORAGE_PALETTE_UINT8;
        storage.voxels = new_ps;

        return;
    }

    if(storage.type == VOXEL_STORAGE_PALETTE_UINT8) {
        auto *ps = reinterpret_cast<PaletteStorageU8 *>(storage.voxels);

        if(voxel == NULL_VOXEL) {
            const auto pdx = ps->indices[index];

            // Figure out if the voxel we're trying to replace
            // is unique in the storage; non-unique voxels only
            // get their palette index reset to zero and that's it
            for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
                if((i != index) && (ps->indices[i] == pdx)) {
                    ps->indices[index] = 0;
                    return;
                }
            }

            ps->indices[pdx] = 0;
            ps->palette[pdx - 1] = NULL_VOXEL;
            return;
        }

        // Figure out if there is an existing index that
        // resolves to the voxel we want to place in the chunk
        for(size_t i = 0; i < ps->palette.size(); ++i) {
            if(voxel == ps->palette.at(i)) {
                ps->indices[index] = i + 1;
                return;
            }
        }

        // There isn't one; now figure out if there's a free
        // index in the palette that we can link with the voxel
        for(size_t i = 0; i < ps->palette.size(); ++i) {
            if(ps->palette.at(i) == NULL_VOXEL) {
                ps->palette[i] = voxel;
                ps->indices[index] = i + 1;
                return;
            }
        }

        // There isn't one; at this point just give up
        // and add a new voxel to the palette
        ps->palette.push_back(voxel);

        if(ps->palette.size() >= UINT8_MAX) {
            auto *new_ps = new PaletteStorageU16();
            new_ps->palette.swap(ps->palette);

            // Googling didn't work so here's a slow
            // and horribly optimized way of copying one
            // array to another. FIXME: maybe use std::copy?
            for(size_t i = 0; i < CHUNK_VOLUME; ++i)
                new_ps->indices[i] = ps->indices[i];
            new_ps->indices[index] = new_ps->palette.size();

            storage.type = VOXEL_STORAGE_PALETTE_UINT16;
            storage.voxels = new_ps;

            delete ps;

            return;
        }

        ps->indices[index] = ps->palette.size();
        return;
    }

    if(storage.type == VOXEL_STORAGE_PALETTE_UINT16) {
        auto *ps = reinterpret_cast<PaletteStorageU8 *>(storage.voxels);

        if(voxel == NULL_VOXEL) {
            const auto pdx = ps->indices[index];

            // Figure out if the voxel we're trying to replace
            // is unique in the storage; non-unique voxels only
            // get their palette index reset to zero and that's it
            for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
                if((i != index) && (ps->indices[i] == pdx)) {
                    ps->indices[index] = 0;
                    return;
                }
            }

            ps->indices[pdx] = 0;
            ps->palette[pdx - 1] = NULL_VOXEL;
            return;
        }

        // Figure out if there is an existing index that
        // resolves to the voxel we want to place in the chunk
        for(size_t i = 0; i < ps->palette.size(); ++i) {
            if(voxel == ps->palette.at(i)) {
                ps->indices[index] = i + 1;
                return;
            }
        }

        // There isn't one; now figure out if there's a free
        // index in the palette that we can link with the voxel
        for(size_t i = 0; i < ps->palette.size(); ++i) {
            if(ps->palette.at(i) == NULL_VOXEL) {
                ps->palette[i] = voxel;
                ps->indices[index] = i + 1;
                return;
            }
        }

        // There isn't one; at this point just give up
        // and add a new voxel to the palette
        ps->palette.push_back(voxel);
        ps->indices[index] = ps->palette.size();

        return;
    }
}

void voxel_storage::fill(VoxelStorage &storage, Voxel voxel)
{
    switch(storage.type) {
        case VOXEL_STORAGE_PALETTE_UINT8:
            delete reinterpret_cast<PaletteStorageU8 *>(storage.voxels);
            break;
        case VOXEL_STORAGE_PALETTE_UINT16:
            delete reinterpret_cast<PaletteStorageU16 *>(storage.voxels);
            break;
    }

    storage.type = VOXEL_STORAGE_SINGLE_VOXEL;
    storage.voxels = reinterpret_cast<void *>(voxel);
}
