
#include <ri/RenderPass.h>

#include <util/math.h>

namespace ri
{
RenderPass::RenderPass(const ri::DeviceContext& device, const AttachmentParams* attachments, size_t attachmentsCount)
    : m_device(detail::getVkHandle(device))
{
    std::vector<VkAttachmentDescription> colorAttachments(attachmentsCount);
    std::vector<VkAttachmentReference>   colorAttachmentRefs(attachmentsCount);

    m_clearValues.resize(attachmentsCount);
    for (size_t i = 0; i < attachmentsCount; ++i)
    {
        const auto& attachmentParam = attachments[i];
        auto&       colorAttachment = colorAttachments[i];
        assert(attachmentParam.format != ColorFormat::eUndefined);
        colorAttachment.format = (VkFormat)attachmentParam.format;
        assert(math::isPowerOfTwo(attachmentParam.samples) && attachmentParam.samples <= 64);
        colorAttachment.samples = (VkSampleCountFlagBits)attachmentParam.samples;
        colorAttachment.loadOp  = (VkAttachmentLoadOp)attachmentParam.colorLoad;
        colorAttachment.storeOp =
            attachmentParam.colorSore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = (VkAttachmentLoadOp)attachmentParam.stencilLoad;
        colorAttachment.stencilStoreOp =
            attachmentParam.stencilStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // setup the default clear value
        {
            ClearValue& clear = m_clearValues[i];
            switch (attachmentParam.format.get())
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

    // TODO: add support for multi subpasses, must also add VkSubpassDependency

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

    RI_CHECK_RESULT_MSG("couldn't create render pass") =
        vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_handle);
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(m_device, m_handle, nullptr);
}

}  // namespace ri
