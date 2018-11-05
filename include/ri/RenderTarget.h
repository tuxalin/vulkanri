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
        AttachmentParams();
        AttachmentParams(const Texture* texture, bool takeOwnership = false);

        const Texture* texture;
        bool           takeOwnership = false;
        union {
            ComponentSwizzle rgbaSwizzle[4];
            struct
            {
                ComponentSwizzle redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle;
            };
        };
    };

    RenderTarget(const DeviceContext& device, const RenderPass& pass, const AttachmentParams& attachment);
    RenderTarget(const DeviceContext& device, const RenderPass& pass, const AttachmentParams* attachments,
                 size_t attachmentsCount);
    RenderTarget(const DeviceContext& device, const RenderPass& pass, const std::vector<AttachmentParams>& attachments);
    ~RenderTarget();

    const Sizei& size() const;

private:
    void createAttachments(const AttachmentParams* attachments, size_t attachmentsCount);

private:
    VkDevice                    m_device = VK_NULL_HANDLE;
    std::vector<VkImageView>    m_imageViews;
    std::vector<const Texture*> m_textures;
    Sizei                       m_size;
};

inline RenderTarget::AttachmentParams::AttachmentParams()
    : texture(nullptr)
    , rgbaSwizzle {ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity,  //
                   ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity}
{
}

inline RenderTarget::AttachmentParams::AttachmentParams(const Texture* texture, bool takeOwnership /*= false*/)
    : texture(texture)
    , takeOwnership(takeOwnership)
    , rgbaSwizzle {ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity,  //
                   ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity}
{
}

inline RenderTarget::RenderTarget(const DeviceContext& device, const RenderPass& pass,
                                  const AttachmentParams& attachment)
    : RenderTarget(device, pass, &attachment, 1)
{
}

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
