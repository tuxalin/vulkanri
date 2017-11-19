
#include <ri/RenderPass.h>

#include "ri_internal_get_handle.h"
#include <util\math.h>

namespace ri
{
RenderPass::RenderPass(const ri::DeviceContext& device, const std::vector<AttachmentParams>& attachments)
    : m_logicalDevice(detail::getVkHandle(device))
{
    static_assert(offsetof(ri::RenderPass, m_handle) == offsetof(ri::detail::RenderPass, m_handle),
                  "INVALID_CLASS_LAYOUT");

    std::vector<VkAttachmentDescription> colorAttachments(attachments.size());
    std::vector<VkAttachmentReference>   colorAttachmentRefs(attachments.size());

    size_t i = 0;
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

    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = 0;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    RI_CHECK_RESULT() = vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_handle);
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(m_logicalDevice, m_handle, nullptr);
}
}  // namespace ri
