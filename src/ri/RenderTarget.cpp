
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
    framebufferInfo.attachmentCount         = m_imageViews.size();
    framebufferInfo.pAttachments            = m_imageViews.data();
    framebufferInfo.width                   = m_size.width;
    framebufferInfo.height                  = m_size.height;
    framebufferInfo.layers                  = 1;

    RI_CHECK_RESULT_MSG("couldn't create framebuffer") =
        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_handle);
}

RenderTarget::~RenderTarget()
{
    vkDestroyFramebuffer(m_device, m_handle, nullptr);
    size_t i = 0;
    for (auto imageView : m_imageViews)
    {
        if (m_imageViewsOwnership[i++])
            vkDestroyImageView(m_device, imageView, nullptr);
    }
    for (auto texture : m_textures)
        delete texture;
}

void RenderTarget::createAttachments(const AttachmentParams* attachments, size_t attachmentsCount)
{
    for (size_t i = 0; i < attachmentsCount; ++i)
    {
        const auto& attachmentParam = attachments[i];
        assert(attachmentParam.texture);

        const VkImageView textureView = detail::getImageViewHandle(*attachmentParam.texture);
        if (textureView)
        {
            m_imageViews.push_back(textureView);
            m_imageViewsOwnership.push_back(false);
            continue;
        }

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                 = detail::getVkHandle(*attachmentParam.texture);
        createInfo.viewType              = (VkImageViewType)attachmentParam.texture->type();
        createInfo.format                = (VkFormat)attachmentParam.texture->format();
        createInfo.components.r          = (VkComponentSwizzle)attachmentParam.redSwizzle;
        createInfo.components.g          = (VkComponentSwizzle)attachmentParam.greenSwizzle;
        createInfo.components.b          = (VkComponentSwizzle)attachmentParam.blueSwizzle;
        createInfo.components.a          = (VkComponentSwizzle)attachmentParam.alphaSwizzle;
        // no mipmapping or layers

        const VkImageAspectFlags flags = detail::getImageAspectFlags((VkFormat)attachmentParam.texture->format());
        createInfo.subresourceRange.aspectMask     = flags;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        m_imageViews.emplace_back();
        auto& imageView = m_imageViews.back();
        RI_CHECK_RESULT_MSG("couldn't create image view for render target") =
            vkCreateImageView(m_device, &createInfo, nullptr, &imageView);

        if (attachmentParam.takeOwnership)
            m_textures.push_back(attachmentParam.texture);
        m_imageViewsOwnership.push_back(true);
    }
}
}
