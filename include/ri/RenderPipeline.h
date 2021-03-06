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
        struct PushParams
        {
            ShaderStage stages;
            uint32_t    offset, size;

            PushParams() {}
            PushParams(ShaderStage stages, uint32_t offset, uint32_t size)
                : stages(stages)
                , offset(offset)
                , size(size)
            {
            }
        };

        PrimitiveTopology primitiveTopology = PrimitiveTopology::eTriangles;
        bool              primitiveRestart  = false;
        float             lineWidth         = 1.f;
        CullMode          cullMode          = CullMode::eBack;
        bool              frontFaceCW       = true;
        PolygonMode       polygonMode       = PolygonMode::eNormal;

        // Enable/Disable color writing
        bool colorWriteEnable = true;
        // Enable/Disable rasterize stage
        bool rasterizeEnable = true;

        // Blending
        BlendFactor    blendSrcFactor      = BlendFactor::eSrc_Alpha;
        BlendFactor    blendDstFactor      = BlendFactor::eOne_Minus_Src_Alpha;
        BlendOperation blendOperation      = BlendOperation::eAdd;
        BlendFactor    blendAlphaSrcFactor = BlendFactor::eOne;
        BlendFactor    blendAlphaDstFactor = BlendFactor::eOne;
        BlendOperation blendAlphaOperation = BlendOperation::eAdd;
        bool           blend               = false;

        // Depth
        bool             depthTestEnable         = false;
        bool             depthWriteEnable        = false;
        bool             depthClampEnable        = false;
        bool             depthBiasEnable         = false;
        float            depthBiasConstantFactor = 0.0f;
        float            depthBiasClamp          = 0.0f;
        float            depthBiasSlopeFactor    = 0.0f;
        bool             depthBoundsTestEnable   = false;
        float            depthMinBounds          = 0.0f;
        float            depthMaxBounds          = 1.0f;
        CompareOperation depthCompareOp          = CompareOperation::eLessOrEqual;

        // Stencil
        bool           stencilTestEnable = false;
        StencilOpState stencilFrontState = {};
        StencilOpState stencilBackState  = {};

        // MSAA
        bool     sampleShadingEnable  = false;
        float    minSampleShading     = 1.f;
        uint32_t rasterizationSamples = 1;

        // Other
        uint32_t tesselationPatchControlPoints = 0;

        VertexDescription* vertexDescription = nullptr;
        // What subpass to use from the render pass.
        uint32_t activeSubpassIndex = 0;
        // The dynamic states of the pipeline.
        // @note Any dynamic states that are marked must after be explicitly be set with their commands, as the
        // initial/constant values will be ignored.
        std::vector<DynamicState> dynamicStates;
        // Specifying one or more descriptor set layout bindings.
        std::vector<DescriptorSetLayout> descriptorLayouts;
        // The goal of derivative pipelines is that they be cheaper to create using the parent as a starting point, and
        // that it be more efficient (on either host or device) to switch/bind between children of the same parent.
        const RenderPipeline* pipelineDerivative      = nullptr;
        int                   pipelineDerivativeIndex = -1;

        std::vector<PushParams> pushConstants;
    };

    struct DynamicState
    {
        void setViewport(CommandBuffer& buffer, const Sizei& viewportSize,  //
                         int32_t viewportX = 0, int32_t viewportY = 0, float minDepth = 0.f, float maxDepth = 1.f);

        void setScissor(CommandBuffer& buffer, const Sizei& viewportSize,  //
                        int32_t viewportX = 0, int32_t viewportY = 0);

        void setLineWidth(CommandBuffer& buffer, float lineWidth);

        void setDepthBias(CommandBuffer& buffer, float depthBiasConstantFactor, float depthBiasClamp = 0.f,
                          float depthBiasSlopeFactor = 0.f);

        void setStencilCompareMask(CommandBuffer& buffer, VkStencilFaceFlags faceMask, uint32_t compareMask);

        void setStencilWriteMask(CommandBuffer& buffer, VkStencilFaceFlags faceMask, uint32_t writeMask);

        void setStencilReference(CommandBuffer& buffer, VkStencilFaceFlags faceMask, uint32_t reference);

    private:
        VkViewport m_viewport;
        VkRect2D   m_scissor;
    };

    struct ViewportParam
    {
        ViewportParam(const Sizei& size, int32_t viewportX = 0, int32_t viewportY = 0)
            : viewportSize(size)
            , viewportX(viewportX)
            , viewportY(viewportY)
        {
        }

        Sizei   viewportSize;
        int32_t viewportX;
        int32_t viewportY;
    };

    class ScopedEnable
    {
    public:
        ScopedEnable(const RenderPipeline& pipeline, const ri::RenderTarget& target, ri::CommandBuffer& commandBuffer);
        ~ScopedEnable();

    private:
        const RenderPipeline* m_pipeline;
        ri::CommandBuffer*    m_commandBuffer;
    };

    /// @note Takes ownership of the render pass.
    RenderPipeline(const ri::DeviceContext&  device,          //
                   ri::RenderPass*           pass,            //
                   const ri::ShaderPipeline& shaderPipeline,  //
                   const CreateParams&       params,          //
                   const Sizei& viewportSize, int32_t viewportX = 0, int32_t viewportY = 0);
    RenderPipeline(const ri::DeviceContext&  device,          //
                   ri::RenderPass&           pass,            //
                   const ri::ShaderPipeline& shaderPipeline,  //
                   const CreateParams&       params,          //
                   const Sizei& viewportSize, int32_t viewportX = 0, int32_t viewportY = 0);
    ~RenderPipeline();

    ri::RenderPass&       defaultPass();
    const ri::RenderPass& defaultPass() const;
    /// @note To use it you Must create the pipeline with specific dynamic states.
    DynamicState& dynamicState();

    void bind(const CommandBuffer& buffer) const;
    /// @note Also binds the pipeline and the default render pass.
    void begin(const CommandBuffer& buffer, const RenderTarget& target) const;
    /// Ends the  default render pass
    void end(const CommandBuffer& buffer) const;

    void pushConstants(const void* src, ShaderStage stages, size_t offset, size_t size, CommandBuffer& buffer);
    template <typename T>
    void pushConstants(const T& data, ShaderStage stages, size_t offset, CommandBuffer& buffer);

    /// @note Can use pipeline derivative index for faster creation.
    static void create(const ri::DeviceContext&                          device,            //
                       const std::vector<const ri::RenderPass*>&         pipelinesPass,     //
                       const std::vector<const ri::ShaderPipeline*>&     pipelinesShaders,  //
                       const std::vector<RenderPipeline::CreateParams>&  pipelinesParams,
                       const std::vector<RenderPipeline::ViewportParam>& pipelinesViewportParams,
                       const std::vector<DescriptorSetLayout>&           descriptorLayouts,  //
                       std::vector<RenderPipeline*>&                     pipelines);
    static void create(const ri::DeviceContext&                         device,            //
                       const std::vector<const ri::RenderPass*>&        pipelinesPass,     //
                       const std::vector<const ri::ShaderPipeline*>&    pipelinesShaders,  //
                       const std::vector<RenderPipeline::CreateParams>& pipelinesParams,
                       const RenderPipeline::ViewportParam&             viewportParam,
                       const std::vector<DescriptorSetLayout>&          descriptorLayouts,  //
                       std::vector<RenderPipeline*>&                    pipelines);

private:
    struct PipelineCreateData
    {
        PipelineCreateData(const ri::RenderPass& pass, const CreateParams& params, const ViewportParam& viewportParam)
            : vertexInput(getVertexInputInfo(params))
            , inputAssembly(getInputAssemblyInfo(params))
            , viewport(getViewportFrom(viewportParam))
            , scissor(getScissorFrom(viewportParam))
            , viewportState(getViewportStateInfo(viewport, scissor))
            , rasterizer(getRasterizerInfo(params))
            , multisampling(getMultisamplingInfo(params))
            , colorBlendAttachment(getColorBlendAttachmentInfo(params))
            , colorBlending(getColorBlendingInfo(params, colorBlendAttachment))
            , dynamicState(getDynamicStateInfo(params))
            , depthStencil(getDepthStencilInfo(pass, params))
            , tesselation(getTesselationStateInfo(params))
        {
        }

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
        VkPipelineDepthStencilStateCreateInfo  depthStencil;
        VkPipelineTessellationStateCreateInfo  tesselation;
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

    static VkPipelineLayout createLayout(const VkDevice device, const CreateParams& params,
                                         const std::vector<VkDescriptorSetLayout>& descriptorLayouts);

    static VkViewport                             getViewportFrom(const RenderPipeline::ViewportParam& viewportParam);
    static VkRect2D                               getScissorFrom(const RenderPipeline::ViewportParam& viewportParam);
    static VkPipelineVertexInputStateCreateInfo   getVertexInputInfo(const CreateParams& params);
    static VkPipelineInputAssemblyStateCreateInfo getInputAssemblyInfo(const CreateParams& params);
    static VkPipelineViewportStateCreateInfo getViewportStateInfo(const VkViewport& viewport, const VkRect2D& scissor);
    static VkPipelineRasterizationStateCreateInfo getRasterizerInfo(const CreateParams& params);
    static VkPipelineMultisampleStateCreateInfo   getMultisamplingInfo(const CreateParams& params);
    static VkPipelineColorBlendAttachmentState    getColorBlendAttachmentInfo(const CreateParams& params);
    static VkPipelineColorBlendStateCreateInfo    getColorBlendingInfo(
           const CreateParams& params, const VkPipelineColorBlendAttachmentState& colorBlendAttachment);
    static VkPipelineDynamicStateCreateInfo      getDynamicStateInfo(const CreateParams& params);
    static VkPipelineDepthStencilStateCreateInfo getDepthStencilInfo(const ri::RenderPass& pass,
                                                                     const CreateParams&   params);
    static VkPipelineTessellationStateCreateInfo getTesselationStateInfo(const CreateParams& params);

    static VkGraphicsPipelineCreateInfo getPipelineCreateInfo(const RenderPass&         pass,            //
                                                              const ShaderPipeline&     shaderPipeline,  //
                                                              const CreateParams&       params,          //
                                                              const PipelineCreateData& data,
                                                              VkPipelineLayout          pipelineLayout);

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    RenderPass*      m_renderPass     = nullptr;
    bool             m_hasOwnership   = false;
    VkViewport       m_viewport;
    VkRect2D         m_scissor;
    DynamicState     m_dynamicState;

    friend VkPipelineLayout detail::getPipelineLayout(const RenderPipeline& pipeline);
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

inline void RenderPipeline::pushConstants(const void* src, ShaderStage stages, size_t offset, size_t size,
                                          CommandBuffer& buffer)
{
    vkCmdPushConstants(detail::getVkHandle(buffer), m_pipelineLayout, (VkShaderStageFlags)stages, offset, size, src);
}

template <typename T>
void RenderPipeline::pushConstants(const T& data, ShaderStage stages, size_t offset, CommandBuffer& buffer)
{
    vkCmdPushConstants(detail::getVkHandle(buffer), m_pipelineLayout, (VkShaderStageFlags)stages, offset, sizeof(T),
                       &data);
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

inline void RenderPipeline::create(const ri::DeviceContext&                         device,             //
                                   const std::vector<const ri::RenderPass*>&        pipelinesPass,      //
                                   const std::vector<const ri::ShaderPipeline*>&    pipelinesShaders,   //
                                   const std::vector<RenderPipeline::CreateParams>& pipelinesParams,    //
                                   const RenderPipeline::ViewportParam&             viewportParam,      //
                                   const std::vector<DescriptorSetLayout>&          descriptorLayouts,  //
                                   std::vector<RenderPipeline*>&                    pipelines)
{
    create(device, pipelinesPass, pipelinesShaders, pipelinesParams,
           std::vector<RenderPipeline::ViewportParam>({viewportParam}), descriptorLayouts, pipelines);
}

inline void RenderPipeline::DynamicState::setViewport(CommandBuffer& buffer, const Sizei& viewportSize,  //
                                                      int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/,
                                                      float minDepth /*= 0.f*/, float maxDepth /*= 1.f*/)
{
    m_viewport.x        = (float)viewportX;
    m_viewport.y        = (float)viewportY;
    m_viewport.width    = (float)viewportSize.width;
    m_viewport.height   = (float)viewportSize.height;
    m_viewport.minDepth = minDepth;
    m_viewport.maxDepth = maxDepth;
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

inline void RenderPipeline::DynamicState::setDepthBias(CommandBuffer& buffer, float depthBiasConstantFactor,
                                                       float depthBiasClamp /* = 0.f*/,
                                                       float depthBiasSlopeFactor /* = 0.f*/)
{
    vkCmdSetDepthBias(detail::getVkHandle(buffer), depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

inline void RenderPipeline::DynamicState::setStencilCompareMask(CommandBuffer& buffer, VkStencilFaceFlags faceMask,
                                                                uint32_t compareMask)
{
    vkCmdSetStencilCompareMask(detail::getVkHandle(buffer), faceMask, compareMask);
}

inline void RenderPipeline::DynamicState::setStencilWriteMask(CommandBuffer& buffer, VkStencilFaceFlags faceMask,
                                                              uint32_t writeMask)
{
    vkCmdSetStencilWriteMask(detail::getVkHandle(buffer), faceMask, writeMask);
}

inline void RenderPipeline::DynamicState::setStencilReference(CommandBuffer& buffer, VkStencilFaceFlags faceMask,
                                                              uint32_t reference)
{
    vkCmdSetStencilReference(detail::getVkHandle(buffer), faceMask, reference);
}

inline RenderPipeline::ScopedEnable::ScopedEnable(const RenderPipeline& pipeline, const ri::RenderTarget& target,
                                                  ri::CommandBuffer& commandBuffer)
    : m_pipeline(&pipeline)
    , m_commandBuffer(&commandBuffer)
{
    m_pipeline->begin(commandBuffer, target);
}

inline RenderPipeline::ScopedEnable::~ScopedEnable()
{
    m_pipeline->end(*m_commandBuffer);
}

}  // namespace ri
