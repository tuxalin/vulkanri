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
    struct PushParams
    {
        uint32_t offset, size;

        PushParams() {}
        PushParams(uint32_t offset, uint32_t size)
            : offset(offset)
            , size(size)
        {
        }
    };

    ComputePipeline(const ri::DeviceContext& device,            //
                    DescriptorSetLayout      descriptorLayout,  //
                    const ri::ShaderModule&  shaderModule,      //
                    const std::string&       procedureName = "main");

    ComputePipeline(const ri::DeviceContext& device,            //
                    DescriptorSetLayout      descriptorLayout,  //
                    const ri::ShaderModule&  shaderModule,      //
                    const PushParams&        pushConstants,     //
                    const std::string&       procedureName = "main");

    ComputePipeline(const ri::DeviceContext&       device,            //
                    DescriptorSetLayout            descriptorLayout,  //
                    const ri::ShaderModule&        shaderModule,      //
                    const std::vector<PushParams>& pushConstants,     //
                    const std::string&             procedureName = "main");

    ~ComputePipeline();

    void bind(CommandBuffer& buffer) const;
    void bind(CommandBuffer& buffer, const ri::DescriptorSet& descriptor) const;
    void dispatch(CommandBuffer& buffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;

    void pushConstants(const void* src, size_t offset, size_t size, CommandBuffer& buffer);
    template <typename T>
    void pushConstants(const T& data, CommandBuffer& buffer);
    template <typename T>
    void pushConstants(const T& data, size_t offset, CommandBuffer& buffer);

private:
    VkPipelineLayout createLayout(const VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                  const std::vector<PushParams>& pushConstants);

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

inline void ComputePipeline::pushConstants(const void* src, size_t offset, size_t size, CommandBuffer& buffer)
{
    vkCmdPushConstants(detail::getVkHandle(buffer), m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, src);
}

template <typename T>
void ComputePipeline::pushConstants(const T& data, CommandBuffer& buffer)
{
    vkCmdPushConstants(detail::getVkHandle(buffer), m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(T), &data);
}

template <typename T>
void ComputePipeline::pushConstants(const T& data, size_t offset, CommandBuffer& buffer)
{
    vkCmdPushConstants(detail::getVkHandle(buffer), m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, sizeof(T),
                       &data);
}

}  // namespace ri
