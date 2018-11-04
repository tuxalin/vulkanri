
#include <ri/RenderTarget.h>

#include <ri/Texture.h>

namespace ri
{
RenderTarget::RenderTarget(const DeviceContext& device, const RenderPass& pass, const AttachmentParams* attachments,
                           size_t attachmentsCount)
    : m_device(detail::getVkHandle(device))
    , m_size(attachments[0].texture->size())
{
    createAttachments(attachments, attachmentsCount);

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass              = detail::getVkHandle(pass);
    framebufferInfo.attachmentCount         = m_attachments.size();
    framebufferInfo.pAttachments            = m_attachments.data();
    framebufferInfo.width                   = m_size.width;
    framebufferInfo.height                  = m_size.height;
    framebufferInfo.layers                  = 1;

    RI_CHECK_RESULT_MSG("couldn't create framebuffer") =
        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_handle);
}

RenderTarget::~RenderTarget()
{
    vkDestroyFramebuffer(m_device, m_handle, nullptr);
    for (auto imageView : m_attachments)
        vkDestroyImageView(m_device, imageView, nullptr);
}

void RenderTarget::createAttachments(const AttachmentParams* attachments, size_t attachmentsCount)
{
    for (size_t i = 0; i < attachmentsCount; ++i)
    {
        const auto& attachment = attachments[i];
        assert(attachment.texture);

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                 = detail::getVkHandle(*attachment.texture);
        createInfo.viewType              = (VkImageViewType)attachment.texture->type();
        createInfo.format                = (VkFormat)attachment.texture->format();
        createInfo.components.r          = (VkComponentSwizzle)attachment.redSwizzle;
        createInfo.components.g          = (VkComponentSwizzle)attachment.greenSwizzle;
        createInfo.components.b          = (VkComponentSwizzle)attachment.blueSwizzle;
        createInfo.components.a          = (VkComponentSwizzle)attachment.alphaSwizzle;
        // no mipmapping or layers
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        m_attachments.emplace_back();
        auto& imageView = m_attachments.back();
        RI_CHECK_RESULT_MSG("couldn't create image view for render target") =
            vkCreateImageView(m_device, &createInfo, nullptr, &imageView);
    }
}
}
