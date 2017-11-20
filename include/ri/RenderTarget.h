#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;
class RenderPass;
class Texture;

class RenderTarget : util::noncopyable, public detail::RenderObject<VkFramebuffer>
{
public:
    struct AttachmentParams
    {
        AttachmentParams()
            : texture(nullptr)
            , rgbaSwizzle{ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity,  //
                          ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity}
        {
        }

        const Texture* texture;
        ColorFormat    format;
        union {
            ComponentSwizzle rgbaSwizzle[4];
            struct
            {
                ComponentSwizzle redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle;
            };
        };
    };

    RenderTarget(const DeviceContext& device, const RenderPass& pass, const std::vector<AttachmentParams>& attachments);
    ~RenderTarget();

    const Sizei& size() const;

private:
    void createAttachments(const std::vector<AttachmentParams>& attachments);

private:
    VkDevice                 m_device = VK_NULL_HANDLE;
    std::vector<VkImageView> m_attachments;
    Sizei                    m_size;
};

inline const Sizei& RenderTarget::size() const
{
    return m_size;
}

}  // namespace ri
