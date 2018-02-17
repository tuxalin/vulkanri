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
    DescriptorType type;

    DescriptorBinding() {}
    DescriptorBinding(uint32_t index, ShaderStage stageFlags, DescriptorType type)
        : index(index)
        , stageFlags(stageFlags)
        , type(type)
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
    typedef std::pair<DescriptorType, size_t> TypeSize;
    struct CreateLayoutResult
    {
        DescriptorSetLayout layout;
        size_t              index;
    };

    DescriptorPool(const DeviceContext& device, DescriptorType type, size_t maxCount);
    DescriptorPool(const DeviceContext& device, const TypeSize* availableDescriptors, size_t availableDescriptorsCount);
    DescriptorPool(const DeviceContext& device, const std::vector<TypeSize>& availableDescriptors);
    ///@warning The RenderPipeline that use the layout descriptors must be destroyed before the pool.
    ~DescriptorPool();

    DescriptorSet create(uint32_t layoutIndex);
    ///@note Also calls descriptor update.
    DescriptorSet create(uint32_t layoutIndex, const DescriptorSetParams& params);
    void          create(const uint32_t* layoutIndices, size_t layoutIndicesCount, DescriptorSet* descriptors);
    void          create(const std::vector<uint32_t>& layoutIndices, std::vector<DescriptorSet>& descriptors);

    ///@note New layout will be appended.
    CreateLayoutResult createLayout(const DescriptorLayoutParam& param);
    ///@note New layouts will be appended.
    size_t createLayouts(const DescriptorLayoutParam* layoutParams, size_t layoutParamsCount);
    size_t createLayouts(const std::vector<DescriptorLayoutParam>& layoutParams);

    const std::vector<DescriptorSetLayout>& layouts() const;

private:
    VkDevice m_device = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayout> m_descriptorLayouts;
};

inline DescriptorPool::DescriptorPool(const DeviceContext& device, const std::vector<TypeSize>& availableDescriptors)
    : DescriptorPool(device, availableDescriptors.data(), availableDescriptors.size())
{
}

inline void DescriptorPool::create(const std::vector<uint32_t>& layoutIndices, std::vector<DescriptorSet>& descriptors)
{
    descriptors.resize(layoutIndices.size());
    create(layoutIndices.data(), layoutIndices.size(), descriptors.data());
}

inline size_t DescriptorPool::createLayouts(const std::vector<DescriptorLayoutParam>& layoutParams)
{
    createLayouts(layoutParams.data(), layoutParams.size());
}

inline const std::vector<DescriptorSetLayout>& DescriptorPool::layouts() const
{
    return m_descriptorLayouts;
}

}  // namespace ri
