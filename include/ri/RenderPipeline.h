#pragma once

#include <util/noncopyable.h>
#include <ri/RenderPass.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;
class ShaderPipeline;

class RenderPipeline : util::noncopyable
{
public:
    struct CreateParams
    {
        PrimitiveTopology primitiveTopology = PrimitiveTopology::eTriangles;
        bool              primitiveRestart  = false;
        float             lineWidth         = 1.f;
        CullMode          cullMode          = CullMode::eBack;
        PolygonMode       polygonMode       = PolygonMode::eNormal;

        // Enable/Disable color writing
        bool colorWrite = true;
        // Enable/Disable rasterize stage
        bool rasterize = true;

        // Blending
        BlendFactor    blendSrcFactor      = BlendFactor::eSrc_Alpha;
        BlendFactor    blendDstFactor      = BlendFactor::eOne_Minus_Src_Alpha;
        BlendOperation blendOperation      = BlendOperation::eAdd;
        BlendFactor    blendAlphaSrcFactor = BlendFactor::eOne;
        BlendFactor    blendAlphaDstFactor = BlendFactor::eOne;
        BlendOperation blendAlphaOperation = BlendOperation::eAdd;
        bool           blend               = false;

        // Depth
        bool  depthClamp              = false;
        bool  depthBias               = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasClamp          = 0.0f;
        float depthBiasSlopeFactor    = 0.0f;

        uint32_t activeSubpassIndex = 0;
    };

    ///@note Takes ownerwship of the render pass.
    RenderPipeline(const ri::DeviceContext&  device,          //
                   ri::RenderPass*           pass,            //
                   const ri::ShaderPipeline& shaderPipeline,  //
                   const CreateParams&       params,          //
                   const Sizei& viewportSize, int32_t viewportX = 0, int32_t viewportY = 0);
    RenderPipeline(const ri::DeviceContext&  device,          //
                   const ri::RenderPass&     pass,            //
                   const ri::ShaderPipeline& shaderPipeline,  //
                   const CreateParams&       params,          //
                   const Sizei& viewportSize, int32_t viewportX = 0, int32_t viewportY = 0);
    ~RenderPipeline();

    ri::RenderPass&       defaultPass();
    const ri::RenderPass& defaultPass() const;

    ///@note Also binds the pipeline.
    void begin(const CommandBuffer& buffer, const RenderTarget& target) const;
    void end(const CommandBuffer& buffer) const;
    void bind(const CommandBuffer& buffer) const;

private:
    void setViewport(const Sizei& viewportSize, int32_t viewportX, int32_t viewportY);
    void create(const ri::RenderPass& pass, const ri::ShaderPipeline& shaderPipeline, const CreateParams& params);

private:
    VkPipeline       m_handle         = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice         m_logicalDevice  = VK_NULL_HANDLE;
    ri::RenderPass*  m_renderPass     = nullptr;
    VkViewport       m_viewport;
    VkRect2D         m_scissor;
};

inline void RenderPipeline::begin(const CommandBuffer& buffer, const RenderTarget& target) const
{
    assert(m_renderPass);

    m_renderPass->begin(buffer, target);
    bind(buffer);
}

inline void RenderPipeline::end(const CommandBuffer& buffer) const
{
    assert(m_renderPass);
    m_renderPass->end(buffer);
}

inline ri::RenderPass& RenderPipeline::defaultPass()
{
    assert(m_renderPass);
    return *m_renderPass;
}

inline const ri::RenderPass& RenderPipeline::defaultPass() const
{
    assert(m_renderPass);
    return *m_renderPass;
}
}  // namespace ri
