#include "shared/pch.hh"

#include "shared/world/biome_slice.hh"

#include "core/buffer.hh"

// A slice is a handful of different ids repeated over 256 columns, which is
// about as compressible as data gets.
void BiomeSlice::encode(const BiomeSlice& slice, WriteBuffer& buffer)
{
    buffer.write_compressed(std::as_bytes(std::span(slice.data(), constant::CHUNK_AREA)));
}

void BiomeSlice::decode(BiomeSlice& slice, ReadBuffer& buffer)
{
    slice.fill(BIOME_ID_NULL);

    auto raw = buffer.read_compressed();

    if(raw.size() != constant::CHUNK_AREA * sizeof(biome_id_type)) {
        return;
    }

    std::memcpy(slice.data(), raw.data(), raw.size());
}
