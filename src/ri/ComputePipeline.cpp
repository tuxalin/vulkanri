
#include <ri/ComputePipeline.h>

#include <ri/DescriptorSet.h>
#include <ri/ShaderModule.h>

namespace ri
{
namespace detail
{
    VkPipelineLayout getPipelineLayout(const ComputePipeline& pipeline)
    {
        return pipeline.m_pipelineLayout;
    }
}

ComputePipeline::ComputePipeline(const ri::DeviceContext& device,            //
                                 DescriptorSetLayout      descriptorLayout,  //
                                 const ri::ShaderModule&  shaderModule,      //
                                 const std::string&       procedureName)
{
    m_pipelineLayout = createLayout(detail::getVkHandle(device), {descriptorLayout}, {});
    m_device         = detail::getVkHandle(device);

    VkComputePipelineCreateInfo info = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    info.stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage.stage                 = VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module                = detail::getVkHandle(shaderModule);
    info.stage.pName                 = procedureName.c_str();
    info.layout                      = m_pipelineLayout;
    RI_CHECK_RESULT_MSG("couldn't create compute pipeline") =
        vkCreateComputePipelines(detail::getVkHandle(device), VK_NULL_HANDLE, 1, &info, nullptr, &m_handle);
}

ComputePipeline::ComputePipeline(const ri::DeviceContext& device,            //
                                 DescriptorSetLayout      descriptorLayout,  //
                                 const ri::ShaderModule&  shaderModule,      //
                                 const PushParams&        pushConstant,      //
                                 const std::string&       procedureName /* = "main"*/)
    : ComputePipeline(device, descriptorLayout, shaderModule, std::vector<PushParams> {pushConstant}, procedureName)
{
}

ComputePipeline::ComputePipeline(const ri::DeviceContext&       device,            //
                                 DescriptorSetLayout            descriptorLayout,  //
                                 const ri::ShaderModule&        shaderModule,      //
                                 const std::vector<PushParams>& pushConstants,     //
                                 const std::string&             procedureName)
{
    m_pipelineLayout = createLayout(detail::getVkHandle(device), {descriptorLayout}, pushConstants);
    m_device         = detail::getVkHandle(device);

    VkComputePipelineCreateInfo info = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    info.stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage.stage                 = VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module                = detail::getVkHandle(shaderModule);
    info.stage.pName                 = procedureName.c_str();
    info.layout                      = m_pipelineLayout;
    RI_CHECK_RESULT_MSG("couldn't create compute pipeline") =
        vkCreateComputePipelines(detail::getVkHandle(device), VK_NULL_HANDLE, 1, &info, nullptr, &m_handle);
}

ComputePipeline::~ComputePipeline()
{
    vkDestroyPipeline(m_device, m_handle, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

inline VkPipelineLayout ComputePipeline::createLayout(const VkDevice                            device,
                                                      const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                                      const std::vector<PushParams>&            pushConstants)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount             = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts                = descriptorLayouts.data();

    std::vector<VkPushConstantRange> ranges(pushConstants.size());
    for (size_t i = 0; i < ranges.size(); ++i)
    {
        const auto& pushParam = pushConstants[i];
        auto&       range     = ranges[i];
        range.offset          = pushParam.offset;
        range.size            = pushParam.size;
        range.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
    pipelineLayoutInfo.pPushConstantRanges    = ranges.data();

    VkPipelineLayout pipelineLayout;
    RI_CHECK_RESULT_MSG("couldn't create pipeline layout") =
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    return pipelineLayout;
}

void ComputePipeline::bind(CommandBuffer& buffer, const ri::DescriptorSet& descriptor) const
{
    vkCmdBindPipeline(detail::getVkHandle(buffer), VK_PIPELINE_BIND_POINT_COMPUTE, m_handle);
    descriptor.bind(buffer, *this);
}

}  // namespace ri
