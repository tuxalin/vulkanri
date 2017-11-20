#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;

class Texture : util::noncopyable, public detail::RenderObject<VkImage>
{
public:
    Texture(const DeviceContext& device);
    ~Texture();

    TextureType  type() const;
    const Sizei& size() const;

private:
    Texture(VkImage handle, TextureType type, const Sizei& size);

private:
    VkDevice    m_logicalDevice = VK_NULL_HANDLE;
    TextureType m_type;
    Sizei       m_size;

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
