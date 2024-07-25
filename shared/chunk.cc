// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <array>
#include <shared/chunk.hh>
#include <spdlog/spdlog.h>
#include <vector>

struct PaletteEntry final {
    short count {};
    Voxel voxel {};
};

struct PaletteStorage final {
    std::array<std::uint8_t, CHUNK_VOLUME> indices {};
    std::vector<PaletteEntry> palette {};
};

// Fastest and least memory-conserving type
using ArrayStorage = std::array<Voxel, CHUNK_VOLUME>;

static void set_single_fill(Chunk &chunk, Voxel voxel, std::size_t index)
{
    const auto filler = static_cast<Voxel>(reinterpret_cast<std::uintptr_t>(chunk.storage));
    
    if(filler == voxel) {
        // No need to change anything since
        // the chunk is already filled with
        // the same exact voxel type
        return;
    }
    
    // Not the same type, time to morph
    // into the next complexity level storage
    PaletteStorage *palette_storage = new PaletteStorage();
    
    if(filler == NULL_VOXEL) {
        // NULL_VOXEL is a special case for
        // palette storage type. Because NULL_VOXEL
        // is a universal constant, it makes sense
        // to not have it as a palette entry and instead
        // hardwire zero index to NULL_VOXEL
        palette_storage->indices.fill(0x00);
    }
    else {
        PaletteEntry filler_entry = {};
        filler_entry.count = CHUNK_VOLUME - 2;
        filler_entry.voxel = filler;

        palette_storage->palette.push_back(std::move(filler_entry));
        palette_storage->indices.fill(palette_storage->palette.size());
    }
    
    if(voxel == NULL_VOXEL) {
        // Same special NULL_VOXEL edge-case here
        palette_storage->indices[index] = 0x00;
    }
    else {
        PaletteEntry voxel_entry = {};
        voxel_entry.voxel = voxel;
        voxel_entry.count = 1U;
        
        palette_storage->palette.push_back(std::move(voxel_entry));
        palette_storage->indices[index] = palette_storage->palette.size();
    }
    
    chunk.storage_type = ChunkStorageType::Palette;
    chunk.storage = palette_storage;
}

static void set_palette(Chunk &chunk, Voxel voxel, std::size_t index)
{
    PaletteStorage *palette_storage = reinterpret_cast<PaletteStorage *>(chunk.storage);
    std::uint8_t current_pdx = palette_storage->indices[index];
    
    if(current_pdx) {
        PaletteEntry &current_entry = palette_storage->palette[current_pdx];

        if(current_entry.voxel == voxel) {
            // Nothing changes
            return;
        }
        
        if(current_entry.count == 1U) {
            current_entry.voxel = NULL_VOXEL;
            current_entry.count = 0U;
        }
        else {
            // The voxel is not unique in the
            // storage, just decrease use count.
            current_entry.count -= 1U;
        }
                    
        if(voxel == NULL_VOXEL) {
            palette_storage->indices[index] = 0x00;
            return;
        }
    }
    else if(voxel == NULL_VOXEL) {
        // Nothing changes
        return;
    }

    // Try using an existing palette entry
    // for the specified voxel type, if there is one.
    for(std::size_t i = 0; i < palette_storage->palette.size(); ++i) {
        PaletteEntry &entry = palette_storage->palette.at(i);
        
        if(entry.voxel == voxel) {
            palette_storage->indices[index] = i + 1;
            entry.count += 1U;
            return;
        }
    }
    
    // Try occuping a free palette entry
    for(std::size_t i = 0; i < palette_storage->palette.size(); ++i) {
        PaletteEntry &entry = palette_storage->palette.at(i);
        
        if((entry.count == 0U) && (entry.voxel == NULL_VOXEL)) {
            palette_storage->indices[index] = i + 1;
            entry.voxel = voxel;
            entry.count = 1U;
            return;
        }
    }
    
    PaletteEntry voxel_entry = {};
    voxel_entry.voxel = voxel;
    voxel_entry.count = 1U;
    
    // Give up and make a new entry
    palette_storage->palette.push_back(std::move(voxel_entry));
    
    if(palette_storage->palette.size() <= UINT8_MAX) {
        palette_storage->indices[index] = palette_storage->palette.size();
        return;
    }

    // A palette is no longer enough so we morph
    // the storage into a CHUNK_VOLUME-sized Voxel array
    ArrayStorage *array_storage = new ArrayStorage();

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i)
        array_storage->at(i) = palette_storage->palette.at(palette_storage->indices[i] - 1).voxel;
    array_storage->at(index) = voxel;

    chunk.storage_type = ChunkStorageType::Array;
    chunk.storage = array_storage;

    delete palette_storage;
}

static void set_array(Chunk &chunk, Voxel voxel, std::size_t index)
{
    ArrayStorage *array_storage = reinterpret_cast<ArrayStorage *>(chunk.storage);
    array_storage->at(index) = voxel;
}

void Chunk::create_storage(Chunk &chunk)
{
    // Make sure there is no allocated
    // storage present - no memory leaks
    Chunk::destroy_storage(chunk);

    chunk.storage_type = ChunkStorageType::SingleFill;
    chunk.storage = reinterpret_cast<void *>(NULL_VOXEL);
}

void Chunk::create_storage(Chunk &chunk, const Chunk &parent)
{
    switch(parent.storage_type) {
        case ChunkStorageType::SingleFill:
            chunk.storage_type = ChunkStorageType::SingleFill;
            chunk.storage = parent.storage;
            return;
        case ChunkStorageType::Palette:
            chunk.storage_type = ChunkStorageType::Palette;
            chunk.storage = new PaletteStorage(reinterpret_cast<const PaletteStorage *>(parent.storage)[0]);
            return;
        case ChunkStorageType::Array:
            chunk.storage_type = ChunkStorageType::Array;
            chunk.storage = new ArrayStorage(reinterpret_cast<const ArrayStorage *>(parent.storage)[0]);
            return;
        default:
            chunk.storage_type = ChunkStorageType::Unallocated;
            chunk.storage = nullptr;
            return;
    }
}

void Chunk::destroy_storage(Chunk &chunk)
{
    switch(chunk.storage_type) {
        case ChunkStorageType::SingleFill:
            chunk.storage_type = ChunkStorageType::Unallocated;
            chunk.storage = nullptr;
            return;
        case ChunkStorageType::Palette:
            delete reinterpret_cast<PaletteStorage *>(chunk.storage);
            chunk.storage_type = ChunkStorageType::Unallocated;
            chunk.storage = nullptr;
            return;
        case ChunkStorageType::Array:
            delete reinterpret_cast<ArrayStorage *>(chunk.storage);
            chunk.storage_type = ChunkStorageType::Unallocated;
            chunk.storage = nullptr;
            return;
        default:
            chunk.storage_type = ChunkStorageType::Unallocated;
            chunk.storage = nullptr;
            return;
    }
}

Voxel Chunk::get_voxel(const Chunk &chunk, const LocalPos &lpos)
{
    return Chunk::get_voxel(chunk, coord::to_index(lpos));
}

Voxel Chunk::get_voxel(const Chunk &chunk, std::size_t index)
{
    const ArrayStorage *array_storage;
    const PaletteStorage *palette_storage;
    std::size_t palette_index;

    if(index >= CHUNK_VOLUME) {
        // Voxel index out of range;
        // Assume NULL_VOXEL
        return NULL_VOXEL;
    }

    switch(chunk.storage_type) {
        case ChunkStorageType::SingleFill:
            return static_cast<Voxel>(reinterpret_cast<std::uintptr_t>(chunk.storage));
        case ChunkStorageType::Palette:
            palette_storage = reinterpret_cast<const PaletteStorage *>(chunk.storage);
            palette_index = palette_storage->indices[index];
            return palette_index ? palette_storage->palette[palette_index - 1].voxel : NULL_VOXEL;
        case ChunkStorageType::Array:
            array_storage = reinterpret_cast<const ArrayStorage *>(chunk.storage);
            return array_storage->at(index);
    }

    // Assume NULL_VOXEL for any other type
    // including ChunkStorageType::Unallocated
    return NULL_VOXEL;
}

void Chunk::set_voxel(Chunk &chunk, Voxel voxel, const LocalPos &lpos)
{
    Chunk::set_voxel(chunk, voxel, coord::to_index(lpos));
}

void Chunk::set_voxel(Chunk &chunk, Voxel voxel, std::size_t index)
{
    if(index >= CHUNK_VOLUME) {
        // Voxel index out of range
        return;
    }

    switch(chunk.storage_type) {
        case ChunkStorageType::SingleFill:
            set_single_fill(chunk, voxel, index);
            return;
        case ChunkStorageType::Palette:
            set_palette(chunk, voxel, index);
            return;
        case ChunkStorageType::Array:
            set_array(chunk, voxel, index);
            return;
    }
}

void Chunk::fill(Chunk &chunk, Voxel voxel)
{
    Chunk::destroy_storage(chunk);
    chunk.storage_type = ChunkStorageType::SingleFill;
    chunk.storage = reinterpret_cast<void *>(voxel);
}
