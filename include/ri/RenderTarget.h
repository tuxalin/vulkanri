#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;
class RenderPass;
class Texture;

class RenderTarget : util::noncopyable, public RenderObject<VkFramebuffer>
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

    RenderTarget(const DeviceContext& device, const RenderPass& pass, const AttachmentParams* attachments,
                 size_t attachmentsCount);
    RenderTarget(const DeviceContext& device, const RenderPass& pass, const std::vector<AttachmentParams>& attachments);
    ~RenderTarget();

    const Sizei& size() const;

private:
    void createAttachments(const AttachmentParams* attachments, size_t attachmentsCount);

private:
    VkDevice                 m_device = VK_NULL_HANDLE;
    std::vector<VkImageView> m_attachments;
    Sizei                    m_size;
};

inline RenderTarget::RenderTarget(const DeviceContext& device, const RenderPass& pass,
                                  const std::vector<AttachmentParams>& attachments)
    : RenderTarget(device, pass, attachments.data(), attachments.size())
{
}

inline const Sizei& RenderTarget::size() const
{
    return m_size;
}

}  // namespace ri
