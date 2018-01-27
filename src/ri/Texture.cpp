
#include <ri/Texture.h>

#include <ri/DeviceContext.h>

namespace ri
{
namespace detail
{
    const ri::Texture* createReferenceTexture(VkImage handle, int type, const Sizei& size)
    {
        return new ri::Texture(handle, TextureType::from(type), size);
    }
}

namespace
{
    uint32_t findMemoryIndex(const VkPhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter,
                             VkMemoryPropertyFlags flags)
    {
        size_t i   = 0;
        auto found = std::find_if(memProperties.memoryTypes, memProperties.memoryTypes + memProperties.memoryTypeCount,
                                  [typeFilter, flags, &i](const auto& memoryType) {
                                      return (typeFilter & (1 << i++)) && (memoryType.propertyFlags & flags) == flags;
                                  });
        assert(found != (memProperties.memoryTypes + memProperties.memoryTypeCount));
        return i - 1;
    }
}

Texture::Texture(const DeviceContext& device, const TextureParams& params)
    : m_device(detail::getVkHandle(device))
{
    createImage(params);
    allocateMemory(device, params);
}

Texture::Texture(VkImage handle, TextureType type, const Sizei& size)
    : RenderObject<VkImage>(handle)
    , m_type(type)
    , m_size(size)
{
}

Texture::~Texture()
{
    if (m_device)
    {
        vkDestroyImage(m_device, m_handle, nullptr);
        vkFreeMemory(m_device, m_memory, nullptr);
    }
}

inline void Texture::createImage(const TextureParams& params)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = (VkImageType)params.type;
    imageInfo.extent.width      = static_cast<uint32_t>(params.size.width);
    imageInfo.extent.height     = static_cast<uint32_t>(params.size.height);
    imageInfo.extent.depth      = params.depth;
    imageInfo.mipLevels         = params.mipLevels;
    imageInfo.arrayLayers       = params.arrayLevels;
    imageInfo.format            = (VkFormat)params.format;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = (VkImageUsageFlags)params.flags;
    // the image will only be used by one queue family: the one that supports graphics and transfer operations.
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples     = (VkSampleCountFlagBits)params.samples;
    imageInfo.flags       = 0;

    RI_CHECK_RESULT_MSG("failed to create image") = vkCreateImage(m_device, &imageInfo, nullptr, &m_handle);
}

inline void Texture::allocateMemory(const DeviceContext& device, const TextureParams& params)
{
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;

    const VkPhysicalDeviceMemoryProperties& memProperties = detail::getDeviceMemoryProperties(device);
    allocInfo.memoryTypeIndex =
        findMemoryIndex(memProperties, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    RI_CHECK_RESULT_MSG("failed to allocate image memory") = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);

    vkBindImageMemory(m_device, m_handle, m_memory, 0);
}

}  // namespace ri
