#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class DescriptorSet;
struct DescriptorSetParams;

struct DescriptorBinding
{
    uint32_t       index = 0;
    ShaderStage    stageFlags;
    DescriptorType descriptorType = DescriptorType::eUniformBuffer;

    DescriptorBinding() {}
    DescriptorBinding(uint32_t index, ShaderStage stageFlags, DescriptorType descriptorType)
        : index(index)
        , stageFlags(stageFlags)
        , descriptorType(descriptorType)
    {
    }
};

struct DescriptorLayoutParam
{
    std::vector<DescriptorBinding> bindings;

    DescriptorLayoutParam() {}
    DescriptorLayoutParam(const DescriptorBinding& binding)
        : bindings(1, binding)
    {
    }
    DescriptorLayoutParam(std::initializer_list<DescriptorBinding> list)
        : bindings(list.begin(), list.end())
    {
    }
};

class DescriptorPool : util::noncopyable, public RenderObject<VkDescriptorPool>
{
public:
    struct CreateLayoutResult
    {
        DescriptorSetLayout layout;
        size_t              index;
    };

    DescriptorPool(const DeviceContext& device, DescriptorType type, size_t maxCount);
    ///@warning The RenderPipeline that use the layout descriptors must be destroyed before the pool.
    ~DescriptorPool();

    DescriptorSet create(const DescriptorSetParams& params, size_t layoutIndex);
    void create(const std::vector<DescriptorSetParams>& descriptorParams, const std::vector<size_t>& layoutIndices,
                std::vector<DescriptorSet>& descriptors);

    ///@note New layout will be appended.
    CreateLayoutResult createLayout(const DescriptorLayoutParam& param);
    ///@note New layouts will be appended.
    size_t createLayouts(const std::vector<DescriptorLayoutParam>& layoutParams);

    DescriptorType type() const;

private:
    std::vector<VkDescriptorSetLayout> createDescriptorLayouts(const std::vector<DescriptorLayoutParam>& layoutParams);

private:
    VkDevice       m_device = VK_NULL_HANDLE;
    DescriptorType m_type;

    std::vector<VkDescriptorSetLayout> m_descriptorLayouts;
};

inline DescriptorType DescriptorPool::type() const
{
    return m_type;
}

}  // namespace ri
