#include "core/pch.hh"

#include "core/res/image.hh"

#include "core/res/resource.hh"
#include "core/utils/physfs.hh"

static int stbi_physfs_read(void* context, char* data, int size)
{
    return static_cast<int>(PHYSFS_readBytes(reinterpret_cast<PHYSFS_File*>(context), data, size));
}

static void stbi_physfs_skip(void* context, int count)
{
    auto file = reinterpret_cast<PHYSFS_File*>(context);
    PHYSFS_seek(file, PHYSFS_tell(file) + count);
}

static int stbi_physfs_eof(void* context)
{
    return PHYSFS_eof(reinterpret_cast<PHYSFS_File*>(context));
}

static const void* image_load_fn(const char* path, std::uint32_t flags)
{
    assert(path);

    stbi_io_callbacks callbacks;
    callbacks.read = &stbi_physfs_read;
    callbacks.skip = &stbi_physfs_skip;
    callbacks.eof = &stbi_physfs_eof;

    stbi_set_flip_vertically_on_load(bool(flags & RESFLAG_IMG_FLIP));

    auto file = PHYSFS_openRead(path);

    if(file == nullptr) {
        msg::log_warning("{}: {}", path, utils::physfs_error());
        return nullptr;
    }

    int desired_channels;

    if(flags & RESFLAG_IMG_GRAY) {
        desired_channels = STBI_grey;
    }
    else {
        desired_channels = STBI_rgb_alpha;
    }

    int width;
    int height;
    int channels;
    auto pixels = stbi_load_from_callbacks(&callbacks, file, &width, &height, &channels, desired_channels);

    PHYSFS_close(file);

    if(pixels == nullptr) {
        msg::log_warning("{}: {}", path, stbi_failure_reason());
        return nullptr;
    }

    auto image = new Image;
    image->width = width;
    image->height = height;
    image->channels = desired_channels;
    image->pixels = pixels;

    return image;
}

static void image_free_fn(const void* resource)
{
    assert(resource);

    auto image = reinterpret_cast<const Image*>(resource);
    stbi_image_free(image->pixels);

    delete image;
}

void Image::register_resource(void)
{
    res::register_loader<Image>(&image_load_fn, &image_free_fn);
}
