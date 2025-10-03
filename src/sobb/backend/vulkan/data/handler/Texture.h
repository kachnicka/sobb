#pragma once

#include <berries/util/UidUtil.h>
#include <vLime/Memory.h>
#include <vLime/Transfer.h>

namespace backend::vulkan::data {

struct Texture {
    using ID = uid<Texture>;

    lime::Image img;
    vk::UniqueImageView imgView;

    void reset()
    {
        imgView.reset();
        img.reset();
    }
};

class TextureHandler {
public:
    lime::MemoryManager& memory;
    lime::Transfer& transfer;

private:
    uidVector<Texture> textures;
    Texture::ID defaultTexture;

public:
    TextureHandler(lime::MemoryManager& memory, lime::Transfer& transfer)
        : memory(memory)
        , transfer(transfer)
    {
        // magenta texture for failure cases
        u32 color = 0xffc311f4;
        defaultTexture = add(1, 1, &color, vk::Format::eR8G8B8A8Unorm);
    }

    Texture& operator[](Texture::ID id)
    {
        return textures[id];
    }

    Texture const& operator[](Texture::ID id) const
    {
        return textures[id];
    }

    Texture::ID add(uint32_t x, uint32_t y, void const* data, vk::Format format)
    {
        if (x * y == 0)
            return {};

        Texture::ID tId { textures.add() };
        auto& texture { textures[tId] };

        vk::ImageCreateInfo cInfo {
            .flags = {},
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = vk::Extent3D { x, y, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        };
        texture.img = memory.alloc({ .memoryUsage = lime::DeviceMemoryUsage::eDeviceOptimal }, cInfo, std::format("texture_{}", tId.get()).c_str());

        auto const size { x * y * lime::img_util::BytesPerPixel(cInfo.format) };
        transfer.StageToDeviceSync(data, size, texture.img);

        vk::ImageViewCreateInfo cInfoView;
        cInfoView.image = texture.img.get();
        cInfoView.viewType = vk::ImageViewType::e2D;
        cInfoView.format = cInfo.format;
        cInfoView.subresourceRange.aspectMask = lime::img_util::Aspect(cInfo.format);
        cInfoView.subresourceRange.levelCount = 1;
        cInfoView.subresourceRange.layerCount = 1;
        texture.imgView = lime::check(memory.d.createImageViewUnique(cInfoView));

        return tId;
    }

    void remove(Texture::ID id)
    {
        textures.remove(id);
    }

    [[nodiscard]] vk::ImageView GetDefaultTextureImageView() const
    {
        return textures[defaultTexture].imgView.get();
    }

    [[nodiscard]] lime::Image::Detail GetDefaultTextureDetail() const
    {
        return textures[defaultTexture].img;
    }
};

}
