#pragma once

#include <util/noncopyable.h>
#include <ri/RenderPass.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;
class ShaderPipeline;
class VertexDescription;

class RenderPipeline : util::noncopyable, public RenderObject<VkPipeline>
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

        VertexDescription* vertexDescription = nullptr;
        // What subpass to use from the render pass.
        uint32_t activeSubpassIndex = 0;
        // The dynamic states of the pipeline.
        // @note Any dynamic states that are marked must after be explictily be set with their commands, as the
        // initial/constant values will be ignored.
        std::vector<DynamicState> dynamicStates;
        // The goal of derivative pipelines is that they be cheaper to create using the parent as a starting point, and
        // that it be more efficient (on either host or device) to switch/bind between children of the same parent.
        const RenderPipeline* pipelineDerivative      = nullptr;
        int                   pipelineDerivativeIndex = -1;
    };

    struct DynamicState
    {
        void setViewport(CommandBuffer& buffer, const Sizei& viewportSize,  //
                         int32_t viewportX = 0, int32_t viewportY = 0);

        void setScissor(CommandBuffer& buffer, const Sizei& viewportSize,  //
                        int32_t viewportX = 0, int32_t viewportY = 0);

        void setLineWidth(CommandBuffer& buffer, float lineWidth);

    private:
        VkViewport m_viewport;
        VkRect2D   m_scissor;
    };

    struct ViewportParam
    {
        ViewportParam(const Sizei& size, int32_t viewportX = 0, int32_t viewportY = 0)
            : viewportSize(viewportSize)
            , viewportX(viewportX)
            , viewportY(viewportY)
        {
        }

        Sizei   viewportSize;
        int32_t viewportX;
        int32_t viewportY;
    };

    /// @note Takes ownerwship of the render pass.
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
    /// @note To use it you Must create the pipeline with specific dynamic states.
    DynamicState& dynamicState();

    /// @note Also binds the pipeline.
    void begin(const CommandBuffer& buffer, const RenderTarget& target) const;
    void end(const CommandBuffer& buffer) const;
    void bind(const CommandBuffer& buffer) const;

    /// @note Can use pipeline derivative index for faster creation.
    static void create(const ri::DeviceContext&                          device,            //
                       const std::vector<const ri::RenderPass*>&         pipelinesPass,     //
                       const std::vector<const ri::ShaderPipeline*>&     pipelinesShaders,  //
                       const std::vector<RenderPipeline::CreateParams>&  pipelinesParams,
                       const std::vector<RenderPipeline::ViewportParam>& pipelinesViewportParams,
                       std::vector<RenderPipeline*>&                     pipelines);
    static void create(const ri::DeviceContext&                         device,            //
                       const std::vector<const ri::RenderPass*>&        pipelinesPass,     //
                       const std::vector<const ri::ShaderPipeline*>&    pipelinesShaders,  //
                       const std::vector<RenderPipeline::CreateParams>& pipelinesParams,
                       const RenderPipeline::ViewportParam&             viewportParam,
                       std::vector<RenderPipeline*>&                    pipelines);

private:
    struct PipelineCreateData
    {
        PipelineCreateData(const CreateParams& params, const ViewportParam& viewportParam);

        VkPipelineVertexInputStateCreateInfo   vertexInput;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        VkViewport                             viewport;
        VkRect2D                               scissor;
        VkPipelineViewportStateCreateInfo      viewportState;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineMultisampleStateCreateInfo   multisampling;
        VkPipelineColorBlendAttachmentState    colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo    colorBlending;
        VkPipelineDynamicStateCreateInfo       dynamicState;
    };

    RenderPipeline(const ri::DeviceContext& device, VkPipeline handle, VkPipelineLayout layout,
                   const VkViewport& viewport, const VkRect2D& scissor)
        : RenderObject<VkPipeline>(handle)
        , m_pipelineLayout(layout)
        , m_device(detail::getVkHandle(device))
        , m_viewport(viewport)
        , m_scissor(scissor)
    {
    }

    static VkPipelineLayout createLayout(const VkDevice device, const CreateParams& params);

    static VkPipelineVertexInputStateCreateInfo   getVertexInputInfo(const CreateParams& params);
    static VkPipelineInputAssemblyStateCreateInfo getInputAssemblyInfo(const CreateParams& params);
    static VkPipelineViewportStateCreateInfo getViewportStateInfo(const VkViewport& viewport, const VkRect2D& scissor);
    static VkPipelineRasterizationStateCreateInfo getRasterizerInfo(const CreateParams& params);
    static VkPipelineMultisampleStateCreateInfo   getMultisamplingInfo(const CreateParams& params);
    static VkPipelineColorBlendAttachmentState    getColorBlendAttachmentInfo(const CreateParams& params);
    static VkPipelineColorBlendStateCreateInfo    getColorBlendingInfo(
           const CreateParams& params, const VkPipelineColorBlendAttachmentState& colorBlendAttachment);
    static VkPipelineDynamicStateCreateInfo getDynamicStateInfo(const CreateParams& params);

    static VkGraphicsPipelineCreateInfo getPipelineCreateInfo(const RenderPass&         pass,            //
                                                              const ShaderPipeline&     shaderPipeline,  //
                                                              const CreateParams&       params,          //
                                                              const PipelineCreateData& data,
                                                              VkPipelineLayout          pipelineLayout);

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    RenderPass*      m_renderPass     = nullptr;
    VkViewport       m_viewport;
    VkRect2D         m_scissor;
    DynamicState     m_dynamicState;
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

inline ri::RenderPipeline::DynamicState& RenderPipeline::dynamicState()
{
    return m_dynamicState;
}

inline void RenderPipeline::bind(const CommandBuffer& buffer) const
{
    vkCmdBindPipeline(detail::getVkHandle(buffer), VK_PIPELINE_BIND_POINT_GRAPHICS, m_handle);
}

inline void RenderPipeline::create(const ri::DeviceContext&                         device,            //
                                   const std::vector<const ri::RenderPass*>&        pipelinesPass,     //
                                   const std::vector<const ri::ShaderPipeline*>&    pipelinesShaders,  //
                                   const std::vector<RenderPipeline::CreateParams>& pipelinesParams,
                                   const RenderPipeline::ViewportParam&             viewportParam,
                                   std::vector<RenderPipeline*>&                    pipelines)
{
    create(device, pipelinesPass, pipelinesShaders, pipelinesParams,
           std::vector<RenderPipeline::ViewportParam>({viewportParam}), pipelines);
}

inline void RenderPipeline::DynamicState::setViewport(CommandBuffer& buffer, const Sizei& viewportSize,  //
                                                      int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/)
{
    m_viewport.x      = (float)viewportX;
    m_viewport.y      = (float)viewportY;
    m_viewport.width  = (float)viewportSize.width;
    m_viewport.height = (float)viewportSize.height;
    vkCmdSetViewport(detail::getVkHandle(buffer), 0, 1, &m_viewport);
}

inline void RenderPipeline::DynamicState::setScissor(CommandBuffer& buffer, const Sizei& viewportSize,  //
                                                     int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/)
{
    m_scissor.offset = {viewportX, viewportY};
    m_scissor.extent = {viewportSize.width, viewportSize.height};
    vkCmdSetScissor(detail::getVkHandle(buffer), 0, 1, &m_scissor);
}

inline void RenderPipeline::DynamicState::setLineWidth(CommandBuffer& buffer, float lineWidth)
{
    vkCmdSetLineWidth(detail::getVkHandle(buffer), lineWidth);
}

}  // namespace ri
