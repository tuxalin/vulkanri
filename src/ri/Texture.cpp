
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

Texture::Texture(const DeviceContext& device)
    : m_device(detail::getVkHandle(device))
{
    // TODO: implemetation
}

Texture::Texture(VkImage handle, TextureType type, const Sizei& size)
    : detail::RenderObject<VkImage>(handle)
    , m_type(type)
    , m_size(size)
{
}

Texture::~Texture()
{
    if (m_device)
        vkDestroyImage(m_device, m_handle, nullptr);
}
}  // namespace ri
