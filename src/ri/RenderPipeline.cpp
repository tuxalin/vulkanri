
#include <ri/RenderPipeline.h>

#include <ri/RenderPass.h>
#include <ri/ShaderPipeline.h>

namespace ri
{
RenderPipeline::RenderPipeline(const ri::DeviceContext&  device,          //
                               ri::RenderPass*           pass,            //
                               const ri::ShaderPipeline& shaderPipeline,  //
                               const CreateParams&       params,          //
                               const Sizei& viewportSize, int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/)
    : m_logicalDevice(detail::getVkHandle(device))
    , m_renderPass(pass)
{
    setViewport(viewportSize, viewportX, viewportY);
    create(*pass, shaderPipeline, params);
}

RenderPipeline::RenderPipeline(const ri::DeviceContext&  device,          //
                               const ri::RenderPass&     pass,            //
                               const ri::ShaderPipeline& shaderPipeline,  //
                               const CreateParams&       params,          //
                               const Sizei& viewportSize, int32_t viewportX /*= 0*/, int32_t viewportY /*= 0*/)
    : m_logicalDevice(detail::getVkHandle(device))
{
    setViewport(viewportSize, viewportX, viewportY);
    create(pass, shaderPipeline, params);
}

RenderPipeline::~RenderPipeline()
{
    delete m_renderPass;
    vkDestroyPipeline(m_logicalDevice, m_handle, nullptr);
    vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
}

void RenderPipeline::setViewport(const Sizei& viewportSize, int32_t viewportX, int32_t viewportY)
{
    m_viewport.x        = (float)viewportX;
    m_viewport.y        = (float)viewportY;
    m_viewport.width    = (float)viewportSize.width;
    m_viewport.height   = (float)viewportSize.height;
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;
    m_scissor.offset    = {viewportX, viewportY};
    m_scissor.extent    = {viewportSize.width, viewportSize.height};
}

inline void RenderPipeline::create(const ri::RenderPass& pass, const ri::ShaderPipeline& shaderPipeline,
                                   const CreateParams& params)
{
    // setup vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount        = 0;
    vertexInputInfo.pVertexBindingDescriptions           = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount      = 0;
    vertexInputInfo.pVertexAttributeDescriptions         = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = (VkPrimitiveTopology)params.primitiveTopology;
    inputAssembly.primitiveRestartEnable                 = params.primitiveRestart;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1;
    viewportState.pViewports                        = &m_viewport;
    viewportState.scissorCount                      = 1;
    viewportState.pScissors                         = &m_scissor;

    // setup rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode                            = (VkPolygonMode)params.polygonMode;
    rasterizer.lineWidth                              = params.lineWidth;
    rasterizer.cullMode                               = (VkCullModeFlags)params.cullMode;
    rasterizer.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.rasterizerDiscardEnable                = !params.rasterize;
    // depth
    rasterizer.depthClampEnable        = params.depthClamp;
    rasterizer.depthBiasEnable         = params.depthBias;
    rasterizer.depthBiasConstantFactor = params.depthBiasConstantFactor;
    rasterizer.depthBiasClamp          = params.depthBiasClamp;
    rasterizer.depthBiasSlopeFactor    = params.depthBiasSlopeFactor;

    // multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};  // TODO: expose, disabled for now
    multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable                  = VK_FALSE;
    multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading                     = 1.0f;
    multisampling.pSampleMask                          = nullptr;
    multisampling.alphaToCoverageEnable                = VK_FALSE;
    multisampling.alphaToOneEnable                     = VK_FALSE;

    // blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    const VkColorComponentFlags         writeAllMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.colorWriteMask      = params.colorWrite ? writeAllMask : 0;
    colorBlendAttachment.blendEnable         = params.blend;
    colorBlendAttachment.srcColorBlendFactor = (VkBlendFactor)params.blendSrcFactor;
    colorBlendAttachment.dstColorBlendFactor = (VkBlendFactor)params.blendDstFactor;
    colorBlendAttachment.colorBlendOp        = (VkBlendOp)params.blendOperation;
    colorBlendAttachment.srcAlphaBlendFactor = (VkBlendFactor)params.blendAlphaSrcFactor;
    colorBlendAttachment.dstAlphaBlendFactor = (VkBlendFactor)params.blendAlphaDstFactor;
    colorBlendAttachment.alphaBlendOp        = (VkBlendOp)params.blendAlphaOperation;

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

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount                = params.dynamicStates.size();
    dynamicState.pDynamicStates = reinterpret_cast<const VkDynamicState*>(params.dynamicStates.data());

    // TODO: will add support later
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount             = 0;
    pipelineLayoutInfo.pSetLayouts                = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount     = 0;
    pipelineLayoutInfo.pPushConstantRanges        = 0;

    assert(!m_pipelineLayout);
    RI_CHECK_RESULT() = vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    const auto& shaderStageInfos              = detail::getStageCreateInfos(shaderPipeline);
    pipelineInfo.stageCount                   = shaderStageInfos.size();
    pipelineInfo.pStages                      = shaderStageInfos.data();
    pipelineInfo.pVertexInputState            = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState          = &inputAssembly;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterizer;
    pipelineInfo.pMultisampleState            = &multisampling;
    pipelineInfo.pDepthStencilState           = nullptr;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = &dynamicState;

    pipelineInfo.layout     = m_pipelineLayout;
    pipelineInfo.renderPass = detail::getVkHandle(pass);
    assert(params.activeSubpassIndex < pass.subpassCount());
    pipelineInfo.subpass = params.activeSubpassIndex;

    // TODO: add support for derived pipelines
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = -1;

    assert(!m_handle);
    RI_CHECK_RESULT() =
        vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handle);
}

}  // namespace ri
