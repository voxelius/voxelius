#include "client/pch.hh"

#include "client/res/texture_2d.hh"

#include "core/res/image.hh"
#include "core/res/resource.hh"

static const void* texture_2d_load_fn(const char* path, std::uint32_t flags)
{
    assert(path);

    std::uint32_t image_flags = 0;

    if(flags & RESFLAG_TEX_FLIP) {
        image_flags |= RESFLAG_IMG_FLIP;
    }

    if(flags & RESFLAG_TEX_GRAY) {
        image_flags |= RESFLAG_IMG_GRAY;
    }

    auto image = res::load_by_path<Image>(path, image_flags);

    if(image == nullptr) {
        msg::log_warning("{}: failed to load image", path);
        return nullptr;
    }

    GLenum format_internal;
    GLenum format_source;

    if(flags & RESFLAG_TEX_GRAY) {
        format_internal = GL_R8;
        format_source = GL_RED;
    }
    else {
        format_internal = GL_RGBA8;
        format_source = GL_RGBA;
    }

    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    glTexImage2D(GL_TEXTURE_2D, 0, format_internal, image->width, image->height, 0, format_source, GL_UNSIGNED_BYTE, image->pixels);

    if(flags & RESFLAG_TEX_NEAREST) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    auto texture = new Texture2D;
    texture->width = image->width;
    texture->height = image->height;
    texture->handle = handle;
    texture->imgui = static_cast<ImTextureID>(texture->handle);
    return texture;
}

static void texture_2d_free_fn(const void* resource)
{
    auto texture = reinterpret_cast<const Texture2D*>(resource);
    glDeleteTextures(1, &texture->handle);
    delete texture;
}

void Texture2D::register_resource(void)
{
    res::register_loader<Texture2D>(&texture_2d_load_fn, &texture_2d_free_fn);
}
