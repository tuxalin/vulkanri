#pragma once

#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;

class Texture
{
public:
    Texture(const DeviceContext& device);
    ~Texture();

    TextureType  type() const;
    const Sizei& size() const;

private:
    Texture(VkImage handle, TextureType type, const Sizei& size);

private:
    VkImage     m_handle        = VK_NULL_HANDLE;
    VkDevice    m_logicalDevice = VK_NULL_HANDLE;
    TextureType m_type;
    Sizei       m_size;

    template <class DetailRenderClass, class RenderClass>
    friend auto           detail::getVkHandleImpl(const RenderClass& obj);
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
