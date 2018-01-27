#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;

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
};

class Texture : util::noncopyable, public RenderObject<VkImage>
{
public:
    Texture(const DeviceContext& device, const TextureParams& params);
    ~Texture();

    TextureType  type() const;
    const Sizei& size() const;

private:
    // create a reference texture
    Texture(VkImage handle, TextureType type, const Sizei& size);

    void createImage(const TextureParams& params);
    void allocateMemory(const DeviceContext& device, const TextureParams& params);

private:
    VkDevice       m_device = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
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
