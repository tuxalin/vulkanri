
#include <ri/RenderPipeline.h>

#include <ri/RenderPass.h>
#include <ri/ShaderPipeline.h>
#include <ri/VertexDescription.h>

namespace ri
{
namespace detail
{
    VkPipelineLayout getPipelineLayout(const RenderPipeline& pipeline)
    {
        return pipeline.m_pipelineLayout;
    }
}

RenderPipeline::RenderPipeline(const ri::DeviceContext&  device,          //
                               ri::RenderPass*           pass,            //
                               const ri::ShaderPipeline& shaderPipeline,  //
                               const CreateParams&       params,          //
                               const Sizei& viewportSize, int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/)
    : RenderPipeline(device, *pass, shaderPipeline, params, viewportSize, viewportX, viewportY)
{
    m_hasOwnership = true;
}

RenderPipeline::RenderPipeline(const ri::DeviceContext&  device,          //
                               ri::RenderPass&           pass,            //
                               const ri::ShaderPipeline& shaderPipeline,  //
                               const CreateParams&       params,          //
                               const Sizei& viewportSize, int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/)
    : m_device(detail::getVkHandle(device))
    , m_renderPass(&pass)
{
    const ViewportParam      viewportParam(viewportSize, viewportX, viewportY);
    const PipelineCreateData data(pass, params, viewportParam);

    m_viewport = getViewportFrom(viewportParam);
    m_scissor  = getScissorFrom(viewportParam);

    m_pipelineLayout = createLayout(m_device, params, params.descriptorLayouts);
    const VkGraphicsPipelineCreateInfo info =
        getPipelineCreateInfo(pass, shaderPipeline, params, data, m_pipelineLayout);

    RI_CHECK_RESULT_MSG("couldn't create render pipeline") =
        vkCreateGraphicsPipelines(detail::getVkHandle(device), VK_NULL_HANDLE, 1, &info, nullptr, &m_handle);
}

RenderPipeline::~RenderPipeline()
{
    if (m_hasOwnership)
        delete m_renderPass;
    vkDestroyPipeline(m_device, m_handle, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

void RenderPipeline::create(const ri::DeviceContext&                          device,                   //
                            const std::vector<const ri::RenderPass*>&         pipelinesPass,            //
                            const std::vector<const ri::ShaderPipeline*>&     pipelinesShaders,         //
                            const std::vector<RenderPipeline::CreateParams>&  pipelinesParams,          //
                            const std::vector<RenderPipeline::ViewportParam>& pipelinesViewportParams,  //
                            const std::vector<DescriptorSetLayout>&           descriptorLayouts,        //
                            std::vector<RenderPipeline*>&                     pipelines)
{
    assert(!pipelinesPass.empty());
    assert(!pipelinesViewportParams.empty());
    assert(pipelinesParams.size() == pipelinesShaders.size());
    assert(pipelinesPass.size() == pipelinesShaders.size());

    std::vector<VkPipeline>         pipelineHandles(pipelinesParams.size());
    std::vector<VkPipelineLayout>   pipelineLayoutHandles(pipelinesParams.size());
    std::vector<PipelineCreateData> pipelineCreateData;
    pipelineCreateData.reserve(pipelinesParams.size());
    std::vector<VkGraphicsPipelineCreateInfo> pipelineInfos;
    pipelineInfos.reserve(pipelinesParams.size());

    for (size_t i = 0; i < pipelinesParams.size(); ++i)
    {
        const auto& params = pipelinesParams[i];
        pipelineCreateData.emplace_back(*pipelinesPass[i], params,
                                        pipelinesViewportParams[std::min(i, pipelinesViewportParams.size() - 1)]);
        const auto& data   = pipelineCreateData.back();
        const auto  layout = createLayout(detail::getVkHandle(device), params, descriptorLayouts);
        pipelineLayoutHandles.push_back(layout);
        pipelineInfos.push_back(
            getPipelineCreateInfo(*pipelinesPass[i], *pipelinesShaders[i], pipelinesParams[i], data, layout));
    }

    RI_CHECK_RESULT_MSG("couldn't create multiple render pipelines") =
        vkCreateGraphicsPipelines(detail::getVkHandle(device), VK_NULL_HANDLE, pipelineInfos.size(),
                                  pipelineInfos.data(), nullptr, pipelineHandles.data());

    pipelines.resize(pipelinesParams.size());
    for (size_t i = 0; i < pipelineHandles.size(); ++i)
    {
        auto        handle       = pipelineHandles[i];
        auto        layoutHandle = pipelineLayoutHandles[i];
        const auto& data         = pipelineCreateData[i];
        pipelines[i]             = new RenderPipeline(device, handle, layoutHandle, data.viewport, data.scissor);
    }
}

inline VkViewport RenderPipeline::getViewportFrom(const RenderPipeline::ViewportParam& viewportParam)
{
    VkViewport viewport;
    viewport.x        = (float)viewportParam.viewportX;
    viewport.y        = (float)viewportParam.viewportY;
    viewport.width    = (float)viewportParam.viewportSize.width;
    viewport.height   = (float)viewportParam.viewportSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

inline VkRect2D RenderPipeline::getScissorFrom(const RenderPipeline::ViewportParam& viewportParam)
{
    VkRect2D scissor;
    scissor.offset = {viewportParam.viewportX, viewportParam.viewportY};
    scissor.extent = {viewportParam.viewportSize.width, viewportParam.viewportSize.height};
    return scissor;
}

inline VkPipelineVertexInputStateCreateInfo RenderPipeline::getVertexInputInfo(const CreateParams& params)
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (params.vertexDescription)
    {
        const auto& vertexBindingDescriptions           = detail::getBindingDescriptions(*params.vertexDescription);
        vertexInputInfo.vertexBindingDescriptionCount   = vertexBindingDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions      = vertexBindingDescriptions.data();
        const auto& vertexAttributeDescriptions         = detail::getAttributeDescriptons(*params.vertexDescription);
        vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributeDescriptions.data();
    }
    else
    {
        vertexInputInfo.vertexAttributeDescriptionCount = vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions                                                      = nullptr;
        vertexInputInfo.pVertexAttributeDescriptions                                                    = nullptr;
    }
    return vertexInputInfo;
}

inline VkPipelineInputAssemblyStateCreateInfo RenderPipeline::getInputAssemblyInfo(const CreateParams& params)
{
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = (VkPrimitiveTopology)params.primitiveTopology;
    inputAssembly.primitiveRestartEnable                 = params.primitiveRestart;

    return inputAssembly;
}

inline VkPipelineViewportStateCreateInfo RenderPipeline::getViewportStateInfo(const VkViewport& viewport,
                                                                              const VkRect2D&   scissor)
{
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1;
    viewportState.pViewports                        = &viewport;
    viewportState.scissorCount                      = 1;
    viewportState.pScissors                         = &scissor;

    return viewportState;
}

inline VkPipelineRasterizationStateCreateInfo RenderPipeline::getRasterizerInfo(const CreateParams& params)
{
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode                            = (VkPolygonMode)params.polygonMode;
    rasterizer.lineWidth                              = params.lineWidth;
    rasterizer.cullMode                               = (VkCullModeFlags)params.cullMode;
    rasterizer.frontFace               = params.frontFaceCW ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.rasterizerDiscardEnable = !params.rasterizeEnable;
    // depth
    rasterizer.depthClampEnable        = params.depthClampEnable;
    rasterizer.depthBiasEnable         = params.depthBiasEnable;
    rasterizer.depthBiasConstantFactor = params.depthBiasConstantFactor;
    rasterizer.depthBiasClamp          = params.depthBiasClamp;
    rasterizer.depthBiasSlopeFactor    = params.depthBiasSlopeFactor;

    return rasterizer;
}

inline VkPipelineMultisampleStateCreateInfo RenderPipeline::getMultisamplingInfo(const CreateParams& params)
{
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable                  = params.sampleShadingEnable;
    multisampling.rasterizationSamples                 = (VkSampleCountFlagBits)params.rasterizationSamples;
    multisampling.minSampleShading                     = params.minSampleShading;
    multisampling.pSampleMask                          = nullptr;
    multisampling.alphaToCoverageEnable                = VK_FALSE;
    multisampling.alphaToOneEnable                     = VK_FALSE;

    return multisampling;
}

inline VkPipelineColorBlendAttachmentState RenderPipeline::getColorBlendAttachmentInfo(const CreateParams& params)
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    const VkColorComponentFlags         writeAllMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.colorWriteMask      = params.colorWriteEnable ? writeAllMask : 0;
    colorBlendAttachment.blendEnable         = params.blend;
    colorBlendAttachment.srcColorBlendFactor = (VkBlendFactor)params.blendSrcFactor;
    colorBlendAttachment.dstColorBlendFactor = (VkBlendFactor)params.blendDstFactor;
    colorBlendAttachment.colorBlendOp        = (VkBlendOp)params.blendOperation;
    colorBlendAttachment.srcAlphaBlendFactor = (VkBlendFactor)params.blendAlphaSrcFactor;
    colorBlendAttachment.dstAlphaBlendFactor = (VkBlendFactor)params.blendAlphaDstFactor;
    colorBlendAttachment.alphaBlendOp        = (VkBlendOp)params.blendAlphaOperation;

    return colorBlendAttachment;
}

inline VkPipelineColorBlendStateCreateInfo RenderPipeline::getColorBlendingInfo(
    const CreateParams& params, const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
{
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable                       = VK_FALSE;
    colorBlending.logicOp                             = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount                     = 1;
    colorBlending.pAttachments                        = &colorBlendAttachment;
    colorBlending.blendConstants[0]                   = 0.0f;
    colorBlending.blendConstants[1]                   = 0.0f;
    colorBlending.blendConstants[2]                   = 0.0f;
    colorBlending.blendConstants[3]                   = 0.0f;

    return colorBlending;
}

inline VkPipelineDynamicStateCreateInfo RenderPipeline::getDynamicStateInfo(const CreateParams& params)
{
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    if (!params.dynamicStates.empty())
    {
        dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = params.dynamicStates.size();
        dynamicState.pDynamicStates    = reinterpret_cast<const VkDynamicState*>(params.dynamicStates.data());
    }

    return dynamicState;
}

inline VkPipelineDepthStencilStateCreateInfo RenderPipeline::getDepthStencilInfo(const ri::RenderPass& pass,
                                                                                 const CreateParams&   params)
{
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};

    const bool hasDepth =
        std::find_if(pass.attachments().begin(), pass.attachments().end(), [](const auto& attachment) {
            return attachment.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }) != pass.attachments().end();
    if (hasDepth)
    {
        depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable  = params.depthTestEnable;
        depthStencil.depthWriteEnable = params.depthWriteEnable;
        depthStencil.depthCompareOp   = (VkCompareOp)params.depthCompareOp;

        depthStencil.depthBoundsTestEnable = params.depthBoundsTestEnable;
        depthStencil.minDepthBounds        = params.depthMinBounds;
        depthStencil.maxDepthBounds        = params.depthMaxBounds;

        depthStencil.stencilTestEnable = params.stencilTestEnable;
        depthStencil.front             = params.stencilFrontState;
        depthStencil.back              = params.stencilBackState;
    }
    return depthStencil;
}

inline VkPipelineTessellationStateCreateInfo RenderPipeline::getTesselationStateInfo(const CreateParams& params)
{
    VkPipelineTessellationStateCreateInfo tesselation = {};
    tesselation.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tesselation.patchControlPoints                    = params.tesselationPatchControlPoints;
    return tesselation;
}

inline VkPipelineLayout RenderPipeline::createLayout(const VkDevice device, const CreateParams& params,
                                                     const std::vector<VkDescriptorSetLayout>& descriptorLayouts)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount             = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts                = descriptorLayouts.data();

    std::vector<VkPushConstantRange> ranges(params.pushConstants.size());
    for (size_t i = 0; i < ranges.size(); ++i)
    {
        const auto& pushParam = params.pushConstants[i];
        auto&       range     = ranges[i];
        range.offset          = pushParam.offset;
        range.size            = pushParam.size;
        range.stageFlags      = (VkShaderStageFlags)pushParam.stages;
    }
    pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
    pipelineLayoutInfo.pPushConstantRanges    = ranges.data();

    VkPipelineLayout pipelineLayout;
    RI_CHECK_RESULT_MSG("couldn't create pipeline layout") =
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    return pipelineLayout;
}

inline VkGraphicsPipelineCreateInfo RenderPipeline::getPipelineCreateInfo(const ri::RenderPass&     pass,
                                                                          const ri::ShaderPipeline& shaderPipeline,
                                                                          const CreateParams&       params,
                                                                          const PipelineCreateData& data,
                                                                          VkPipelineLayout          pipelineLayout)
{
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    const auto& shaderStageInfos              = detail::getStageCreateInfos(shaderPipeline);
    pipelineInfo.stageCount                   = shaderStageInfos.size();
    pipelineInfo.pStages                      = shaderStageInfos.data();
    pipelineInfo.pVertexInputState            = &data.vertexInput;
    pipelineInfo.pInputAssemblyState          = &data.inputAssembly;
    pipelineInfo.pViewportState               = &data.viewportState;
    pipelineInfo.pRasterizationState          = &data.rasterizer;
    pipelineInfo.pMultisampleState            = &data.multisampling;
    pipelineInfo.pColorBlendState             = &data.colorBlending;
    if (data.dynamicState.sType == VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO)
        pipelineInfo.pDynamicState = &data.dynamicState;
    if (data.depthStencil.sType == VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO)
        pipelineInfo.pDepthStencilState = &data.depthStencil;
    if (data.tesselation.patchControlPoints)
        pipelineInfo.pTessellationState = &data.tesselation;

    assert(pipelineLayout);
    pipelineInfo.layout     = pipelineLayout;
    pipelineInfo.renderPass = detail::getVkHandle(pass);
    assert(params.activeSubpassIndex < pass.subpassCount());
    pipelineInfo.subpass = params.activeSubpassIndex;

    if (params.pipelineDerivative)
        pipelineInfo.basePipelineHandle = params.pipelineDerivative->m_handle;
    else
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = params.pipelineDerivativeIndex;

    return pipelineInfo;
}

}  // namespace ri
