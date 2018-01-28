#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;
class Buffer;
class CommandBuffer;

struct SamplerParams
{
    enum FilterType
    {
        eNearest = VK_FILTER_NEAREST,
        eLinear  = VK_FILTER_LINEAR
    };
    enum AddressMode
    {
        // Repeat the texture when going beyond the image dimensions.
        eRepeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        //  Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
        eMirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        // Take the color of the edge closest to the coordinate beyond the image dimensions.
        eClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        // Like clamp to edge, but instead uses the edge opposite to the closest edge.
        eClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        // Return a solid color when sampling beyond the dimensions of the image.
        eMirrorClampToEdge = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
    };

    FilterType  magFilter    = eNearest;
    FilterType  minFilter    = eNearest;
    AddressMode addressModeU = eRepeat;
    AddressMode addressModeV = eRepeat;
    AddressMode addressModeW = eRepeat;

    bool  anisotropyEnable = false;
    float maxAnisotropy    = 0.f;

    bool             compareEnable = false;
    CompareOperation compareOp     = CompareOperation::eAlways;

    FilterType mipmapMode = eLinear;
    float      mipLodBias = 0.f;
    float      minLod     = 0.f;
    float      maxLod     = 0.f;
};

struct TextureParams
{
    enum Samples
    {
        eOne     = VK_SAMPLE_COUNT_1_BIT,
        eTwo     = VK_SAMPLE_COUNT_2_BIT,
        eFour    = VK_SAMPLE_COUNT_4_BIT,
        eEight   = VK_SAMPLE_COUNT_8_BIT,
        eSixteen = VK_SAMPLE_COUNT_16_BIT
    };

    TextureType type   = TextureType::e2D;
    ColorFormat format = ColorFormat::eRGBA;
    ///@see ri:: TextureUsageFlags
    uint32_t flags;
    Sizei    size;
    // Depth of a texture, eg a 3D texture is described as width x height x depth
    uint32_t depth       = 1;
    uint32_t mipLevels   = 1;
    uint32_t arrayLevels = 1;
    Samples  samples     = eOne;

    bool          samplerEnable = false;
    SamplerParams samplerParams;
};

class Texture : util::noncopyable, public RenderObject<VkImage>
{
public:
    enum LayoutType
    {
        eUndefined               = VK_IMAGE_LAYOUT_UNDEFINED,
        eGeneral                 = VK_IMAGE_LAYOUT_GENERAL,
        eColorAttachement        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        eDepthStencilAttachement = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        eShaderReadOnly          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        eTransferSrcOptimal      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        eTransferDstOptimal      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ePreinitialized          = VK_IMAGE_LAYOUT_PREINITIALIZED
    };

    struct CopyParams
    {
        // if equal then no transition will be performed
        LayoutType oldLayout = eUndefined;
        LayoutType newLayout = eUndefined;

        int32_t offsetX = 0, offsetY = 0, offsetZ = 0;
        /// @note If zero then will use texture size.
        Sizei    size;
        uint32_t depth = 1;
    };

    Texture(const DeviceContext& device, const TextureParams& params);
    ~Texture();

    TextureType  type() const;
    const Sizei& size() const;

    /// Copy from a staging buffer and issue a transfer command to the given command buffer.
    /// @note It's done asynchronously.
    void copy(const Buffer& src, const CopyParams& params, CommandBuffer& commandBuffer);

private:
    // create a reference texture
    Texture(VkImage handle, TextureType type, const Sizei& size);

    void createImage(const TextureParams& params);
    void createImageView(const TextureParams& params);
    void createSampler(const SamplerParams& params);
    void allocateMemory(const DeviceContext& device, const TextureParams& params);
    void transitionImageLayout(LayoutType oldLayout, LayoutType newLayout, CommandBuffer& commandBuffer);

private:
    VkDevice       m_device  = VK_NULL_HANDLE;
    VkDeviceMemory m_memory  = VK_NULL_HANDLE;
    VkImageView    m_view    = VK_NULL_HANDLE;
    VkSampler      m_sampler = VK_NULL_HANDLE;
    TextureType    m_type;
    Sizei          m_size;

    friend const Texture* detail::createReferenceTexture(VkImage handle, int type, const Sizei& size);
};

inline TextureType Texture::type() const
{
    return m_type;
}

inline const Sizei& Texture::size() const
{
    return m_size;
}
}  // namespace ri
