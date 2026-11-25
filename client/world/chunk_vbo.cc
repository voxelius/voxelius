#include "client/pch.hh"

#include "client/world/chunk_vbo.hh"

#include "client/world/chunk_mesh.hh"

#include <algorithm>

static constexpr std::size_t INITIAL_CAPACITY = 1024U * 1024U;

GLuint chunk_vbo::handle;
std::size_t chunk_vbo::capacity;
std::vector<chunk_vbo::FreeRegion> chunk_vbo::free_list;

static std::size_t s_vertex_limit;

static void grow(std::size_t required)
{
    static std::vector<std::uint8_t> scratch;

    std::size_t new_capacity = chunk_vbo::capacity ? chunk_vbo::capacity : INITIAL_CAPACITY;

    while(new_capacity < required) {
        new_capacity <<= 1;
    }

    auto old_bytes = chunk_vbo::capacity * sizeof(ChunkMeshVertex);
    auto new_bytes = new_capacity * sizeof(ChunkMeshVertex);

    scratch.resize(new_bytes);

    glBindBuffer(GL_ARRAY_BUFFER, chunk_vbo::handle);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(old_bytes), scratch.data());
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(new_bytes), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(old_bytes), scratch.data());

    chunk_vbo::capacity = new_capacity;
}

static void insert_free_region(chunk_vbo::FreeRegion region)
{
    auto& list = chunk_vbo::free_list;

    auto it = std::lower_bound(list.begin(), list.end(), region, [](const auto& a, const auto& b) {
        return a.base < b.base;
    });

    if(it != list.begin()) {
        auto prev = std::prev(it);

        if(prev->base + prev->size == region.base) {
            it = prev;
            it->size += region.size;
        }
        else {
            it = list.insert(it, region);
        }
    }
    else {
        it = list.insert(it, region);
    }

    auto next = std::next(it);

    if(next != list.end() && it->base + it->size == next->base) {
        it->size += next->size;
        list.erase(next);
    }
}

void chunk_vbo::init(void)
{
    capacity = INITIAL_CAPACITY;
    s_vertex_limit = 0;
    free_list.clear();

    glGenBuffers(1, &handle);
    glBindBuffer(GL_ARRAY_BUFFER, handle);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(capacity * sizeof(ChunkMeshVertex)), nullptr, GL_DYNAMIC_DRAW);
}

void chunk_vbo::shutdown(void)
{
    glDeleteBuffers(1, &handle);

    handle = 0;
    capacity = 0;
    s_vertex_limit = 0;
    free_list.clear();
}

std::uint32_t chunk_vbo::allocate(std::size_t vertex_count)
{
    if(vertex_count == 0) {
        return 0;
    }

    auto it = std::find_if(free_list.begin(), free_list.end(), [vertex_count](const auto& region) {
        return region.size >= vertex_count;
    });

    if(it == free_list.end()) {
        if(s_vertex_limit + vertex_count > capacity) {
            grow(s_vertex_limit + vertex_count);
        }

        auto base = static_cast<std::uint32_t>(s_vertex_limit);
        s_vertex_limit += vertex_count;
        return base;
    }
    else {
        auto base = it->base;

        if(it->size == vertex_count) {
            free_list.erase(it);
        }
        else {
            it->base += static_cast<std::uint32_t>(vertex_count);
            it->size -= static_cast<std::uint32_t>(vertex_count);
        }

        return base;
    }
}

void chunk_vbo::free(std::uint32_t base, std::size_t vertex_count)
{
    if(vertex_count) {
        FreeRegion region {};
        region.base = base;
        region.size = static_cast<std::uint32_t>(vertex_count);

        insert_free_region(region);
    }
}

void chunk_vbo::upload(std::uint32_t base, std::span<const ChunkMeshVertex> data)
{
    if(data.size()) {
        auto offset = static_cast<GLintptr>(base * sizeof(ChunkMeshVertex));
        auto length = static_cast<GLsizeiptr>(data.size_bytes());

        glBindBuffer(GL_ARRAY_BUFFER, handle);
        glBufferSubData(GL_ARRAY_BUFFER, offset, length, data.data());
    }
}
