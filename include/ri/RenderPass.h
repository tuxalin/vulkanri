#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;
class RenderTarget;

class RenderPass : util::noncopyable
{
public:
    struct AttachmentParams
    {
        enum AttachmentLoad
        {
            // Preserve the existing contents of the attachment.
            eLoad = VK_ATTACHMENT_LOAD_OP_LOAD,
            // Clear the values to pass's clear values at the start.
            eClear    = VK_ATTACHMENT_LOAD_OP_CLEAR,
            eDontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
        };

        ColorFormat    format      = ColorFormat::eUndefined;
        uint32_t       samples     = 1;
        AttachmentLoad colorLoad   = eClear;
        AttachmentLoad stencilLoad = eDontCare;
        // Rendered contents will be stored in memory and can be read later, otherwise we dont care if stored.
        bool colorSore    = true;
        bool stencilStore = false;
    };

    RenderPass(const ri::DeviceContext& device, const AttachmentParams& attachment);
    ///@note Atachement order must match the layout index in the shaders.
    RenderPass(const ri::DeviceContext& device, const std::vector<AttachmentParams>& attachments);
    ~RenderPass();

    uint32_t    subpassCount() const;
    ClearValue& clearValue(uint32_t attachementIndex);

    void begin(const CommandBuffer& buffer, const RenderTarget& target) const;
    void end(const CommandBuffer& buffer) const;

    void setRenderArea(const Sizei& area, int32_t offsetX = 0, int32_t offsetY = 0);

private:
    VkRenderPass            m_handle        = VK_NULL_HANDLE;
    VkDevice                m_logicalDevice = VK_NULL_HANDLE;
    std::vector<ClearValue> m_clearValues;
    Sizei                   m_renderArea;
    int32_t                 m_renderAreaOffset[2];

    template <class DetailRenderClass, class RenderClass>
    friend auto detail::getVkHandleImpl(const RenderClass& obj);
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

inline ClearValue& RenderPass::clearValue(uint32_t attachementIndex)
{
    assert(m_clearValues.size() > attachementIndex);
    return m_clearValues[attachementIndex];
}

inline void RenderPass::setRenderArea(const Sizei& area, int32_t offsetX /*= 0*/, int32_t offsetY /*= 0*/)
{
    m_renderArea          = area;
    m_renderAreaOffset[0] = offsetX;
    m_renderAreaOffset[1] = offsetY;
}

}  // namespace ri
