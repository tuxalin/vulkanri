
#include <ri/Texture.h>

#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/DeviceContext.h>

namespace ri
{
namespace detail
{
    const ri::Texture* createReferenceTexture(VkImage handle, int type, int format, const Sizei& size)
    {
        return new ri::Texture(handle, TextureType::from(type), ColorFormat::from(format), size);
    }

    VkImageView getImageViewHandle(const ri::Texture& texture)
    {
        return texture.m_view;
    }

    VkImageAspectFlags getImageAspectFlags(VkFormat format)
    {
        VkImageAspectFlags flags;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            flags = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (format == VK_FORMAT_D32_SFLOAT)
        {
            flags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            flags = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        return flags;
    }

    VkImageType getImageType(int type)
    {
        switch (type)
        {
            case ri::TextureType::e1D:
            case ri::TextureType::eArray1D:
                return VK_IMAGE_TYPE_1D;
            case ri::TextureType::e2D:
            case ri::TextureType::eArray2D:
            case ri::TextureType::eCube:
                return VK_IMAGE_TYPE_2D;
            case ri::TextureType::e3D:
                return VK_IMAGE_TYPE_3D;
            default:
                assert(false);
                return VK_IMAGE_TYPE_2D;
        }
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
    , m_format(params.format)
    , m_size(params.size)
    , m_mipLevels(params.mipLevels ? params.mipLevels
                                   : (uint32_t)floor(log2(std::max(params.size.width, params.size.height))) + 1)
    , m_arrayLevels(m_type == TextureType::eCube ? 6 : params.arrayLevels)
{
#ifndef NDEBUG
    const TextureProperties props =
        device.textureProperties(params.format, params.type, TextureTiling::eOptimal, params.flags);
    assert(props.sampleCounts >= params.samples);
    assert(props.maxExtent.width >= params.size.width);
    assert(props.maxExtent.height >= params.size.height);
    assert(props.maxExtent.depth >= params.depth);
    assert(props.maxMipLevels >= m_mipLevels);
    assert(props.maxArrayLayers >= params.arrayLevels);
#endif
    createImage(params);
    allocateMemory(device, params);

    if (params.flags & TextureUsageFlags::eSampled)
    {
        createSampler(params.samplerParams);
        createImageView(params, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    else if (m_format == ColorFormat::eDepth32)
    {
        createImageView(params, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

Texture::Texture(VkImage handle, TextureType type, ColorFormat format, const Sizei& size)
    : RenderObject<VkImage>(handle)
    , m_type(type)
    , m_format(format)
    , m_size(size)
    , m_arrayLevels(0)
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

    const TextureLayoutType dstTransferLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (params.oldLayout != dstTransferLayout)
        transitionImageLayout(params.oldLayout, dstTransferLayout, commandBuffer);

    // copy buffer to image

    assert((m_arrayLevels * m_size.pixelCount() * sizeof(uint32_t)) < src.bytes());

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    VkBufferImageCopy              region  = {};
    region.bufferOffset                    = params.bufferOffset;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = detail::getImageAspectFlags((VkFormat)m_format);
    region.imageSubresource.mipLevel       = params.mipLevel;
    region.imageSubresource.baseArrayLayer = params.baseArrayLayer;
    region.imageSubresource.layerCount     = m_arrayLevels;
    const Sizei size                       = params.size.width == 0 || params.size.height == 0 ? m_size : params.size;
    region.imageOffset                     = {params.offsetX, params.offsetY, params.offsetZ};
    region.imageExtent                     = {size.width, size.height, params.depth};

    bufferCopyRegions.push_back(region);

    vkCmdCopyBufferToImage(detail::getVkHandle(commandBuffer), detail::getVkHandle(src), m_handle,
                           (VkImageLayout)dstTransferLayout, bufferCopyRegions.size(), bufferCopyRegions.data());

    if (dstTransferLayout != params.finalLayout)
        transitionImageLayout(dstTransferLayout, params.finalLayout, false, commandBuffer);
}

void Texture::generateMipMaps(CommandBuffer& commandBuffer)
{
    assert(m_format != ColorFormat::eDepth32 && m_format != ColorFormat::eDepth24Stencil8 &&
           m_format != ColorFormat::eDepth32Stencil8);

    // TODO: handle 2D arrays

    // Copy down mips from n-1 to n
    for (uint32_t i = 1; i < m_mipLevels; i++)
    {
        VkImageBlit imageBlit{};

        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel   = i - 1;
        imageBlit.srcOffsets[1].x           = int32_t(m_size.width >> (i - 1));
        imageBlit.srcOffsets[1].y           = int32_t(m_size.height >> (i - 1));
        imageBlit.srcOffsets[1].z           = 1;

        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel   = i;
        imageBlit.dstOffsets[1].x           = int32_t(m_size.width >> i);
        imageBlit.dstOffsets[1].y           = int32_t(m_size.height >> i);
        imageBlit.dstOffsets[1].z           = 1;

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel            = i;
        mipSubRange.levelCount              = 1;
        mipSubRange.layerCount              = 1;

        // Transition current mip level to transfer destination
        transitionImageLayout(TextureLayoutType::eUndefined, TextureLayoutType::eTransferDstOptimal,  //
                              false, mipSubRange, commandBuffer);

        // Blit from previous level
        vkCmdBlitImage(detail::getVkHandle(commandBuffer),
                       m_handle,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m_handle,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       VK_FILTER_LINEAR);

        // Transition current mip level to transfer source for read in next iteration
        transitionImageLayout(TextureLayoutType::eTransferDstOptimal, TextureLayoutType::eTransferSrcOptimal, false,
                              mipSubRange, commandBuffer);
    }

    // After the loop, all mip layers transfer src, so transition all to shader access
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = m_mipLevels;
    subresourceRange.baseArrayLayer          = 0;
    subresourceRange.layerCount              = 1;
    transitionImageLayout(TextureLayoutType::eTransferSrcOptimal, TextureLayoutType::eShaderReadOnly,  //
                          false, subresourceRange, commandBuffer);
}

inline void Texture::createImage(const TextureParams& params)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = detail::getImageType(params.type.get());
    imageInfo.extent.width      = static_cast<uint32_t>(params.size.width);
    imageInfo.extent.height     = static_cast<uint32_t>(params.size.height);
    imageInfo.extent.depth      = params.depth;
    imageInfo.mipLevels         = m_mipLevels;
    imageInfo.arrayLayers       = m_type == TextureType::eCube ? 6 : params.arrayLevels;
    imageInfo.format            = (VkFormat)params.format;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = (VkImageUsageFlags)params.flags;
    // the image will only be used by one queue family: the one that supports graphics and transfer operations.
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    assert(math::isPowerOfTwo(params.samples));
    imageInfo.samples = (VkSampleCountFlagBits)params.samples;
    imageInfo.flags   = 0;
    if (m_type == TextureType::eCube)
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    RI_CHECK_RESULT_MSG("failed to create image") = vkCreateImage(m_device, &imageInfo, nullptr, &m_handle);
}

void Texture::createImageView(const TextureParams& params, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                 = m_handle;
    viewInfo.viewType              = (VkImageViewType)m_type;
    viewInfo.format                = (VkFormat)params.format;
    viewInfo.components            = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,  //
                           VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = m_type == TextureType::eCube ? 6 : 1;

    RI_CHECK_RESULT_MSG("failed to create image view") = vkCreateImageView(m_device, &viewInfo, nullptr, &m_view);
}

void Texture::createSampler(const SamplerParams& params)
{
    assert(params.minLod <= m_mipLevels);

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
    samplerInfo.maxLod     = static_cast<float>(m_mipLevels);

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

Texture::PipelineBarrierSettings Texture::getPipelineBarrierSettings(TextureLayoutType              oldLayout,
                                                                     TextureLayoutType              newLayout,
                                                                     bool                           readAccess,
                                                                     const VkImageSubresourceRange& subresourceRange)
{
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout            = (VkImageLayout)oldLayout;
    imageMemoryBarrier.newLayout            = (VkImageLayout)newLayout;
    imageMemoryBarrier.image                = m_handle;
    imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange     = subresourceRange;

    VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    switch (oldLayout.get())
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Only valid as initial layout, memory contents are not preserved
            // Can be accessed directly, no source dependency required
            imageMemoryBarrier.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            srcStageFlags                    = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Old layout is transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStageFlags                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
    }
    switch (newLayout.get())
    {
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Transfer source (copy, blit)
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            dstStageFlags                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_GENERAL:
            // Transfer destination (copy, blit)
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstStageFlags                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            if (readAccess)
                imageMemoryBarrier.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            dstStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            if (readAccess)
                imageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Shader read (sampler, input attachment)
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dstStageFlags                    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT;
            break;
    }
    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        dstStageFlags = srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    return std::make_tuple(imageMemoryBarrier, srcStageFlags, dstStageFlags);
}

void Texture::transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout,  //
                                    bool                           readAccess,                 //
                                    VkPipelineStageFlags           srcStageFlags,              //
                                    VkPipelineStageFlags           dstStageFlags,              //
                                    const VkImageSubresourceRange& subresourceRange,           //
                                    CommandBuffer&                 commandBuffer)
{
    const PipelineBarrierSettings settings =
        getPipelineBarrierSettings(oldLayout, newLayout, readAccess, subresourceRange);
    VkImageMemoryBarrier imageMemoryBarrier = std::get<0>(settings);

    vkCmdPipelineBarrier(detail::getVkHandle(commandBuffer), srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1,
                         &imageMemoryBarrier);
}

void Texture::transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout, bool readAccess,
                                    CommandBuffer& commandBuffer)
{
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = detail::getImageAspectFlags((VkFormat)m_format);
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = m_mipLevels;
    subresourceRange.baseArrayLayer          = 0;
    subresourceRange.layerCount              = m_arrayLevels;

    const PipelineBarrierSettings settings =
        getPipelineBarrierSettings(oldLayout, newLayout, readAccess, subresourceRange);
    VkImageMemoryBarrier imageMemoryBarrier = std::get<0>(settings);
    vkCmdPipelineBarrier(detail::getVkHandle(commandBuffer), std::get<1>(settings), std::get<2>(settings), 0, 0,
                         nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    m_layout = newLayout;
}

void Texture::transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout, bool readAccess,
                                    const VkImageSubresourceRange& subresourceRange, CommandBuffer& commandBuffer)
{
    const PipelineBarrierSettings settings =
        getPipelineBarrierSettings(oldLayout, newLayout, readAccess, subresourceRange);
    const VkImageMemoryBarrier imageMemoryBarrier = std::get<0>(settings);

    vkCmdPipelineBarrier(detail::getVkHandle(commandBuffer), std::get<1>(settings), std::get<2>(settings), 0, 0,
                         nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

}  // namespace ri
