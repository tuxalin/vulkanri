#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class RenderPass : util::noncopyable
{
public:
    struct AttachmentParams
    {
        enum AttachmentLoad
        {
            // Preserve the existing contents of the attachment
            eLoad = VK_ATTACHMENT_LOAD_OP_LOAD,
            // Clear the values to a constant at the start
            eClear    = VK_ATTACHMENT_LOAD_OP_CLEAR,
            eDontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
        };

        ColorFormat    format       = ColorFormat::eUndefined;
        uint32_t       samples      = 1;
        AttachmentLoad colorLoad    = eClear;
        AttachmentLoad stencilLoad  = eDontCare;
        bool           colorSore    = true;
        bool           stencilStore = false;
    };

    RenderPass(const ri::DeviceContext& device, const AttachmentParams& attachment);
    ///@note Atachement order must match the layout index in the shaders.
    RenderPass(const ri::DeviceContext& device, const std::vector<AttachmentParams>& attachments);
    ~RenderPass();

    uint32_t subpassCount() const;

private:
    VkRenderPass m_renderPass    = VK_NULL_HANDLE;
    VkDevice     m_logicalDevice = VK_NULL_HANDLE;
};

inline RenderPass::RenderPass(const ri::DeviceContext& device, const AttachmentParams& attachment)
    : RenderPass(device, std::vector<AttachmentParams>({attachment}))
{
}

inline uint32_t RenderPass::subpassCount() const
{
    // TODO: add support for multi subpasses
    return 1;
}
}  // namespace ri
