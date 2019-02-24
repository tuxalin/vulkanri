#pragma once

#include <util/noncopyable.h>
#include <ri/RenderPass.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;
class DeviceContext;
class ShaderModule;
class VertexDescription;
class DescriptorSet;

class ComputePipeline : util::noncopyable, public RenderObject<VkPipeline>
{
public:
    ComputePipeline(const ri::DeviceContext& device,            //
                    DescriptorSetLayout      descriptorLayout,  //
                    const ri::ShaderModule&  shaderModule,      //
                    const std::string&       procedureName = "main");
    ~ComputePipeline();

    void bind(CommandBuffer& buffer) const;
    void bind(CommandBuffer& buffer, const ri::DescriptorSet& descriptor) const;
    void dispatch(CommandBuffer& buffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;

private:
    VkPipelineLayout createLayout(const VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorLayouts);

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;

    friend VkPipelineLayout detail::getPipelineLayout(const ComputePipeline& pipeline);
};

inline void ComputePipeline::bind(CommandBuffer& buffer) const
{
    vkCmdBindPipeline(detail::getVkHandle(buffer), VK_PIPELINE_BIND_POINT_COMPUTE, m_handle);
}

inline void ComputePipeline::dispatch(CommandBuffer& buffer, uint32_t groupCountX, uint32_t groupCountY,
                                      uint32_t groupCountZ) const
{
    vkCmdDispatch(detail::getVkHandle(buffer), groupCountX, groupCountY, groupCountZ);

    // TODO: add image/resource barriers for syncing with other shader stages
}

}  // namespace ri
