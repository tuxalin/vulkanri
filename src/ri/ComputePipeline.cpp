
#include <ri/ComputePipeline.h>

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
    m_pipelineLayout = createLayout(detail::getVkHandle(device), {descriptorLayout});
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
                                                      const std::vector<VkDescriptorSetLayout>& descriptorLayouts)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount             = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts                = descriptorLayouts.data();

    VkPipelineLayout pipelineLayout;
    RI_CHECK_RESULT_MSG("couldn't create pipeline layout") =
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    return pipelineLayout;
}

}  // namespace ri
