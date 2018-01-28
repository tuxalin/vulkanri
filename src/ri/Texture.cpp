
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
    , m_type(params.type)
    , m_size(params.size)
{
    createImage(params);
    allocateMemory(device, params);

    if (params.samplerEnable)
    {
        createSampler(params.samplerParams);
        createImageView(params);
    }
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

        vkDestroyImageView(m_device, m_view, nullptr);
        vkDestroySampler(m_device, m_sampler, nullptr);
    }
}

void Texture::copy(const Buffer& src, const CopyParams& params, CommandBuffer& commandBuffer)
{
    assert(Sizei(params.offsetX + params.size.width, params.offsetY + params.size.height) <= m_size);

    if (params.oldLayout != params.newLayout)
        transitionImageLayout(params.oldLayout, eTransferDstOptimal, commandBuffer);

    // copy buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    const Sizei size   = params.size.width == 0 || params.size.height == 0 ? m_size : params.size;
    region.imageOffset = {params.offsetX, params.offsetY, params.offsetZ};
    region.imageExtent = {size.width, size.height, params.depth};

    vkCmdCopyBufferToImage(detail::getVkHandle(commandBuffer), detail::getVkHandle(src), m_handle,
                           (VkImageLayout)eTransferDstOptimal, 1, &region);

    if (params.oldLayout != params.newLayout)
        transitionImageLayout(eTransferDstOptimal, params.newLayout, commandBuffer);
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

void Texture::createImageView(const TextureParams& params)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                 = m_handle;
    viewInfo.viewType              = (VkImageViewType)m_type;
    viewInfo.format                = (VkFormat)params.format;
    // TODO: expose this
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    RI_CHECK_RESULT_MSG("failed to create image view") = vkCreateImageView(m_device, &viewInfo, nullptr, &m_view);
}

void Texture::createSampler(const SamplerParams& params)
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter           = (VkFilter)params.magFilter;
    samplerInfo.minFilter           = (VkFilter)params.minFilter;
    samplerInfo.addressModeU        = (VkSamplerAddressMode)params.addressModeU;
    samplerInfo.addressModeV        = (VkSamplerAddressMode)params.addressModeV;
    samplerInfo.addressModeW        = (VkSamplerAddressMode)params.addressModeW;
    samplerInfo.anisotropyEnable    = params.anisotropyEnable;
    samplerInfo.maxAnisotropy       = params.maxAnisotropy;

    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = params.compareEnable;
    samplerInfo.compareOp               = (VkCompareOp)params.compareOp;

    samplerInfo.mipmapMode =
        params.mipmapMode == SamplerParams::eLinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = params.mipLodBias;
    samplerInfo.minLod     = params.minLod;
    samplerInfo.maxLod     = params.maxLod;

    RI_CHECK_RESULT_MSG("failed to create texture sampler") =
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
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

void Texture::transitionImageLayout(LayoutType oldLayout, LayoutType newLayout, CommandBuffer& commandBuffer)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout            = (VkImageLayout)oldLayout;
    barrier.newLayout            = (VkImageLayout)newLayout;
    barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                = m_handle;
    // TODO: expose this
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == eUndefined && newLayout == eTransferDstOptimal)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == eTransferDstOptimal && newLayout == eShaderReadOnly)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        // invalid transition
        assert(false);
    }

    vkCmdPipelineBarrier(detail::getVkHandle(commandBuffer), sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);
}

}  // namespace ri
