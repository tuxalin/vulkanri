#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;
class RenderTarget;

SAFE_ENUM_DECLARE(AttachmentLoad,
                  // Preserve the existing contents of the attachment.
                  eLoad = VK_ATTACHMENT_LOAD_OP_LOAD,
                  // Clear the values to pass's clear values at the start.
                  eClear    = VK_ATTACHMENT_LOAD_OP_CLEAR,
                  eDontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE);

class RenderPass : util::noncopyable, public RenderObject<VkRenderPass>
{
public:
    struct AttachmentParams
    {
        ColorFormat    format      = ColorFormat::eUndefined;
        uint32_t       samples     = 1;
        AttachmentLoad colorLoad   = AttachmentLoad::eClear;
        AttachmentLoad stencilLoad = AttachmentLoad::eDontCare;
        // Is true if the attachment is a resolve target.
        bool resolveAttachment = false;
        // Attachment layout
        TextureLayoutType initialLayout = TextureLayoutType::eUndefined;
        TextureLayoutType finalLayout   = TextureLayoutType::ePresentSrc;
        // Rendered contents will be stored in memory and can be read later, otherwise we dont care if stored.
        bool storeColor   = true;
        bool stencilStore = false;
    };
    struct Attachment
    {
        ColorFormat       format;
        uint32_t          samples;
        TextureLayoutType layout;
    };

    RenderPass(const ri::DeviceContext& device, const AttachmentParams& attachment);
    ///@note Attachment order must match the layout index in the shaders.
    RenderPass(const ri::DeviceContext& device, const AttachmentParams* attachments, size_t attachmentsCount);
    RenderPass(const ri::DeviceContext& device, const std::vector<AttachmentParams>& attachments);
    ~RenderPass();

    uint32_t                       subpassCount() const;
    ClearValue&                    clearValue(uint32_t attachementIndex);
    const std::vector<Attachment>& attachments() const;

    void begin(const CommandBuffer& buffer, const RenderTarget& target) const;
    void end(const CommandBuffer& buffer) const;

    void setRenderArea(const Sizei& area, int32_t offsetX = 0, int32_t offsetY = 0);

private:
    VkDevice                m_device = VK_NULL_HANDLE;
    std::vector<ClearValue> m_clearValues;
    Sizei                   m_renderArea;
    int32_t                 m_renderAreaOffset[2];
    std::vector<Attachment> m_attachments;
};

inline RenderPass::RenderPass(const ri::DeviceContext& device, const AttachmentParams& attachment)
    : RenderPass(device, &attachment, 1)
{
}

inline RenderPass::RenderPass(const ri::DeviceContext& device, const std::vector<AttachmentParams>& attachments)
    : RenderPass(device, attachments.data(), attachments.size())
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

inline const std::vector<RenderPass::Attachment>& RenderPass::attachments() const
{
    return m_attachments;
}

inline void RenderPass::setRenderArea(const Sizei& area, int32_t offsetX /*= 0*/, int32_t offsetY /*= 0*/)
{
    m_renderArea          = area;
    m_renderAreaOffset[0] = offsetX;
    m_renderAreaOffset[1] = offsetY;
}

inline void RenderPass::begin(const CommandBuffer& buffer, const RenderTarget& target) const
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

inline void RenderPass::end(const CommandBuffer& buffer) const
{
    vkCmdEndRenderPass(detail::getVkHandle(buffer));
}

}  // namespace ri
