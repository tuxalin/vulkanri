#pragma once

#include <array>
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
        eLinear  = VK_FILTER_LINEAR,
        eCubic   = VK_FILTER_CUBIC_IMG
    };
    enum AddressMode
    {
        /// Repeat the texture when going beyond the image dimensions.
        eRepeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        ///  Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
        eMirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        /// Take the color of the edge closest to the coordinate beyond the image dimensions.
        eClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        /// Like clamp to edge, but instead uses the edge opposite to the closest edge.
        eClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        /// Return a solid color when sampling beyond the dimensions of the image.
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
};

struct TextureParams
{
    TextureType type   = TextureType::e2D;
    ColorFormat format = ColorFormat::eRGBA;
    ///@see ri::TextureUsageFlags
    uint32_t flags = 0;
    Sizei    size;
    /// Depth of a texture, eg. a 3D texture is described as width x height x depth
    uint32_t depth = 1;
    /// @note If zero the it'll auto calculate the levels based on the width and height of the texture.
    uint32_t mipLevels   = 1;
    uint32_t arrayLevels = 1;
    /// @note Must be a power of two.
    uint32_t samples = 1;

    SamplerParams samplerParams;
};

class Texture : util::noncopyable, public RenderObject<VkImage>
{
public:
    struct CopyParams
    {
        // if equal then no transition will be performed
        union {
            struct
            {
                TextureLayoutType oldLayout;
                TextureLayoutType finalLayout;
            };
            std::array<TextureLayoutType, 2> layouts;
        };

        int32_t offsetX = 0, offsetY = 0, offsetZ = 0;
        /// @note If zero then will use texture size.
        Sizei    size;
        uint32_t depth          = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t mipLevel       = 0;
        size_t   bufferOffset   = 0;

        CopyParams()
            : oldLayout(TextureLayoutType::eUndefined)
            , finalLayout(TextureLayoutType::eUndefined)
        {
        }
    };

    Texture(const DeviceContext& device, const TextureParams& params);
    ~Texture();

    TextureType  type() const;
    ColorFormat  format() const;
    const Sizei& size() const;
    bool         isSampled() const;

    /// Copy from a staging buffer and issue a transfer command to the given command buffer.
    /// @note It's done asynchronously.
    void copy(const Buffer& src, const CopyParams& params, CommandBuffer& commandBuffer);
    void generateMipMaps(CommandBuffer& commandBuffer);
    void transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout,  //
                               CommandBuffer& commandBuffer);
    void transitionImageLayout(const std::array<TextureLayoutType, 2>& layouts, CommandBuffer& commandBuffer);
    void transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout, bool readAccess,
                               CommandBuffer& commandBuffer);

private:
    typedef std::tuple<VkImageMemoryBarrier, VkPipelineStageFlags, VkPipelineStageFlags> PipelineBarrierSettings;
    // create a reference texture
    Texture(VkImage handle, TextureType type, ColorFormat format, const Sizei& size);

    void        createImage(const TextureParams& params);
    VkImageView createImageView(VkImageAspectFlags aspectFlags, uint32_t baseMipLevel, uint32_t baseArrayLayer) const;
    void        createSampler(const SamplerParams& params);
    void        allocateMemory(const DeviceContext& device, const TextureParams& params);

    PipelineBarrierSettings getPipelineBarrierSettings(TextureLayoutType              oldLayout,
                                                       TextureLayoutType              newLayout,
                                                       bool                           readAccess,
                                                       const VkImageSubresourceRange& subresourceRange);

    void transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout, bool readAccess,
                               VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags,
                               const VkImageSubresourceRange& subresourceRange, CommandBuffer& commandBuffer);
    void transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout, bool readAccess,
                               const VkImageSubresourceRange& subresourceRange, CommandBuffer& commandBuffer);

private:
    VkDevice          m_device  = VK_NULL_HANDLE;
    VkDeviceMemory    m_memory  = VK_NULL_HANDLE;
    VkImageView       m_view    = VK_NULL_HANDLE;
    VkSampler         m_sampler = VK_NULL_HANDLE;
    TextureType       m_type;
    TextureLayoutType m_layout = TextureLayoutType::eUndefined;
    ColorFormat       m_format;
    Sizei             m_size;
    uint32_t          m_mipLevels;
    uint32_t          m_arrayLevels;

    mutable std::vector<VkImageView> m_extraViews;

    friend const Texture* detail::createReferenceTexture(VkImage handle, int type, int format, const Sizei& size);
    friend detail::TextureDescriptorInfo detail::getTextureDescriptorInfo(const Texture& texture);

    friend VkImageView detail::createExtraImageView(const Texture& texture, uint32_t baseMipLevel,
                                                    uint32_t baseArrayLayer);
    friend VkImageView detail::getImageViewHandle(const ri::Texture& texture);
};

inline TextureType Texture::type() const
{
    return m_type;
}

inline ColorFormat Texture::format() const
{
    return m_format;
}

inline const Sizei& Texture::size() const
{
    return m_size;
}

inline bool Texture::isSampled() const
{
    return m_sampler != VK_NULL_HANDLE;
}

inline void Texture::transitionImageLayout(const std::array<TextureLayoutType, 2>& layouts,
                                           CommandBuffer&                          commandBuffer)
{
    transitionImageLayout(layouts[0], layouts[1], false, commandBuffer);
}

inline void Texture::transitionImageLayout(TextureLayoutType oldLayout, TextureLayoutType newLayout,
                                           CommandBuffer& commandBuffer)
{
    transitionImageLayout(oldLayout, newLayout, false, commandBuffer);
}

}  // namespace ri
