
#include <ri/RenderPass.h>

#include <array>
#include <util/math.h>
namespace ri
{
RenderPass::RenderPass(const ri::DeviceContext& device, const AttachmentParams* attachmentParams,
                       size_t attachmentsCount)
    : m_device(detail::getVkHandle(device))
{
    std::vector<VkAttachmentDescription> attachments(attachmentsCount);
    std::vector<VkAttachmentReference>   colorAttachmentRefs;
    VkAttachmentReference                depthAttachmentRef;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    m_clearValues.resize(attachmentsCount);
    for (size_t i = 0; i < attachmentsCount; ++i)
    {
        const auto& attachmentParam = attachmentParams[i];
        auto&       colorAttachment = attachments[i];
        assert(attachmentParam.format != ColorFormat::eUndefined);
        colorAttachment.format = (VkFormat)attachmentParam.format;
        assert(math::isPowerOfTwo(attachmentParam.samples) && attachmentParam.samples <= 64);
        colorAttachment.samples = (VkSampleCountFlagBits)attachmentParam.samples;
        colorAttachment.loadOp  = (VkAttachmentLoadOp)attachmentParam.colorLoad;
        colorAttachment.storeOp =
            attachmentParam.storeColor ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = (VkAttachmentLoadOp)attachmentParam.stencilLoad;
        colorAttachment.stencilStoreOp =
            attachmentParam.stencilStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

        bool isColorAttachment = true;
        // setup the default clear value
        {
            ClearValue& clear = m_clearValues[i];
            switch (attachmentParam.format.get())
            {
                case ColorFormat::eDepth32:
                case ColorFormat::eDepth32Stencil8:
                case ColorFormat::eDepth24Stencil8:
                    clear.depthStencil = {1.f, 0};
                    isColorAttachment  = false;
                    break;
                default:
                    clear.color.float32[0] = clear.color.float32[1] = clear.color.float32[2] = 0;
                    clear.color.float32[3]                                                   = 1;
                    break;
            }
        }

        colorAttachment.initialLayout = (VkImageLayout)attachmentParam.initialLayout;
        colorAttachment.finalLayout   = (VkImageLayout)attachmentParam.finalLayout;

        m_attachments.emplace_back();
        auto& currentAttachement   = m_attachments.back();
        currentAttachement.format  = attachmentParam.format;
        currentAttachement.samples = attachmentParam.samples;
        currentAttachement.layout  = attachmentParam.finalLayout;

        if (isColorAttachment)
        {
            colorAttachmentRefs.push_back(VkAttachmentReference {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        }
        else
        {
            depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentRef.attachment = i;
        }
    }

    // TODO: add support for multi subpasses, must also add VkSubpassDependency

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentRefs.size();
    subpass.pColorAttachments    = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment =
        depthAttachmentRef.layout == VK_IMAGE_LAYOUT_UNDEFINED ? nullptr : &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = attachments.size();
    renderPassInfo.pAttachments           = attachments.data();
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
