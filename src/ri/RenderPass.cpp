
#include <ri/RenderPass.h>

#include "ri_internal_get_handle.h"
#include <util\math.h>

namespace ri
{
RenderPass::RenderPass(const ri::DeviceContext& device, const std::vector<AttachmentParams>& attachments)
    : m_logicalDevice(detail::getVkHandle(device))
{
    std::vector<VkAttachmentDescription> colorAttachments(attachments.size());
    std::vector<VkAttachmentReference>   colorAttachmentRefs(attachments.size());

    size_t i = 0;
    m_clearValues.resize(attachments.size());
    for (const auto& attachment : attachments)
    {
        auto& colorAttachment = colorAttachments[i];
        assert(attachment.format != ColorFormat::eUndefined);
        colorAttachment.format = (VkFormat)attachment.format;
        assert(math::isPowerOfTwo(attachment.samples) && attachment.samples <= 64);
        colorAttachment.samples = (VkSampleCountFlagBits)attachment.samples;
        colorAttachment.loadOp  = (VkAttachmentLoadOp)attachment.colorLoad;
        colorAttachment.storeOp =
            attachment.colorSore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = (VkAttachmentLoadOp)attachment.stencilLoad;
        colorAttachment.stencilStoreOp =
            attachment.stencilStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // setup the default clear value
        {
            ClearValue& clear = m_clearValues[i];
            switch (attachment.format.get())
            {
                case ColorFormat::eDepth32:
                    clear.depthStencil = {1.f, 0};
                    break;
                default:
                    clear.color.float32[0] = clear.color.float32[1] = clear.color.float32[2] = 0;
                    clear.color.float32[3]                                                   = 1;
                    break;
            }
        }

        // TODO: expose
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference& colorAttachmentRef = colorAttachmentRefs[i];
        colorAttachmentRef.attachment             = i++;
        colorAttachmentRef.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // TODO: add support for multi subpasses

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentRefs.size();
    subpass.pColorAttachments    = colorAttachmentRefs.data();

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = colorAttachments.size();
    renderPassInfo.pAttachments           = colorAttachments.data();
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpass;

    RI_CHECK_RESULT() = vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_handle);
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(m_logicalDevice, m_handle, nullptr);
}

void RenderPass::begin(const CommandBuffer& buffer, const RenderTarget& target) const
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass            = m_handle;
    renderPassInfo.framebuffer           = detail::getVkHandle(target);
    renderPassInfo.renderArea.offset     = {m_renderAreaOffset[0], m_renderAreaOffset[1]};
    renderPassInfo.renderArea.extent     = {m_renderArea.width, m_renderArea.height};

    renderPassInfo.clearValueCount = m_clearValues.size();
    static_assert(sizeof(VkClearValue) == sizeof(ri::ClearValue), "INVALID_RI_CLEAR_VALUE");
    renderPassInfo.pClearValues = reinterpret_cast<const VkClearValue*>(m_clearValues.data());

    // TODO: expose subpass contents
    vkCmdBeginRenderPass(detail::getVkHandle(buffer), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::end(const CommandBuffer& buffer) const
{
    vkCmdEndRenderPass(detail::getVkHandle(buffer));
}

}  // namespace ri
