
#include <array>
#include <ri/DescriptorPool.h>
#include <ri/DescriptorSet.h>

namespace ri
{
DescriptorPool::DescriptorPool(const DeviceContext& device, size_t poolSetSize, DescriptorType type, size_t maxCount,
                               FlagType flags /*= eNone*/)
    : m_device(ri::detail::getVkHandle(device))
{
    VkDescriptorPoolSize poolSize = {};
    poolSize.type                 = (VkDescriptorType)type;
    poolSize.descriptorCount      = maxCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount              = 1;
    poolInfo.pPoolSizes                 = &poolSize;
    poolInfo.maxSets                    = poolSetSize;
    poolInfo.flags                      = (VkDescriptorPoolCreateFlags)flags;

    RI_CHECK_RESULT_MSG("couldn't create descriptor pool") =
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_handle);
}

DescriptorPool::DescriptorPool(const DeviceContext& device, size_t poolSetSize, const TypeSize* availableDescriptors,
                               size_t availableDescriptorsCount, FlagType flags /*= eNone*/)
    : m_device(ri::detail::getVkHandle(device))
{
    assert(availableDescriptors);
    std::array<VkDescriptorPoolSize, DescriptorType::Count> poolSizes;
    for (size_t i = 0; i < availableDescriptorsCount; ++i)
    {
        const auto& typeSize     = availableDescriptors[i];
        auto&       poolSize     = poolSizes[i];
        poolSize.type            = (VkDescriptorType)typeSize.first;
        poolSize.descriptorCount = typeSize.second;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount              = availableDescriptorsCount;
    poolInfo.pPoolSizes                 = poolSizes.data();
    poolInfo.maxSets                    = poolSetSize;
    poolInfo.flags                      = (VkDescriptorPoolCreateFlags)flags;

    RI_CHECK_RESULT_MSG("couldn't create descriptor pool") =
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_handle);
}

DescriptorPool::~DescriptorPool()
{
    for (const auto& descriptorSetLayout : m_descriptorLayouts)
    {
        vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, nullptr);
    }
    vkDestroyDescriptorPool(m_device, m_handle, nullptr);
}

DescriptorSet DescriptorPool::create(uint32_t layoutIndex)
{
    assert(layoutIndex < m_descriptorLayouts.size());

    VkDescriptorSetLayout       layouts[] = {m_descriptorLayouts[layoutIndex]};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = m_handle;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = layouts;

    VkDescriptorSet handle;
    RI_CHECK_RESULT_MSG("couldn't allocate descriptor sets") = vkAllocateDescriptorSets(m_device, &allocInfo, &handle);

    return DescriptorSet(m_device, handle);
}

DescriptorSet DescriptorPool::create(uint32_t layoutIndex, const DescriptorSetParams& params)
{
    DescriptorSet descriptor = create(layoutIndex);
    descriptor.update<8>(params);
    return descriptor;
}

void DescriptorPool::create(const uint32_t* layoutIndices, size_t layoutIndicesCount, DescriptorSet* descriptors)
{
    assert(descriptors);
    std::vector<VkDescriptorSetLayout> layouts(layoutIndicesCount);
    for (size_t i = 0; i < layoutIndicesCount; ++i)
    {
        const auto layoutIndex = layoutIndices[i];
        assert(layoutIndex < m_descriptorLayouts.size());
        layouts[i] = m_descriptorLayouts[layoutIndex];
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = m_handle;
    allocInfo.descriptorSetCount          = layouts.size();
    allocInfo.pSetLayouts                 = layouts.data();

    std::vector<VkDescriptorSet> handles(layoutIndicesCount);
    RI_CHECK_RESULT_MSG("couldn't allocate descriptor sets") =
        vkAllocateDescriptorSets(m_device, &allocInfo, handles.data());

    for (size_t i = 0; i < layoutIndicesCount; ++i)
    {
        descriptors[i] = DescriptorSet(m_device, handles[i]);
    }
}

void DescriptorPool::free(const DescriptorSet& descriptorSet)
{
    VkDescriptorSet descriptorSetHandle = detail::getVkHandle(descriptorSet);
    vkFreeDescriptorSets(m_device, m_handle, 1, &descriptorSetHandle);
}

void DescriptorPool::free(const DescriptorSet* descriptors, uint32_t count)
{
    std::vector<VkDescriptorSet> descriptorSetHandles(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        descriptorSetHandles[i] = detail::getVkHandle(descriptors[i]);
    }

    vkFreeDescriptorSets(m_device, m_handle, count, descriptorSetHandles.data());
}

DescriptorPool::CreateLayoutResult DescriptorPool::createLayout(const DescriptorLayoutParam& descriptorParams)
{
    std::vector<VkDescriptorSetLayoutBinding> bindingInfos;

    for (const auto& binding : descriptorParams.bindings)
    {
        bindingInfos.emplace_back();
        auto& bindingInfo              = bindingInfos.back();
        bindingInfo.binding            = binding.index;
        bindingInfo.descriptorCount    = 1;
        bindingInfo.descriptorType     = (VkDescriptorType)binding.type;
        bindingInfo.stageFlags         = (VkShaderStageFlags)binding.stageFlags;
        bindingInfo.pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount                    = bindingInfos.size();
    layoutInfo.pBindings                       = bindingInfos.data();

    m_descriptorLayouts.emplace_back();
    auto& descriptorSetLayout = m_descriptorLayouts.back();
    RI_CHECK_RESULT_MSG("couldn't create descriptor set layout") =
        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &descriptorSetLayout);

    return CreateLayoutResult({m_descriptorLayouts.back(), m_descriptorLayouts.size() - 1});
}

size_t DescriptorPool::createLayouts(const DescriptorLayoutParam* layoutParams, size_t layoutParamsCount)
{
    assert(layoutParams);

    size_t startIndex = m_descriptorLayouts.size();

    std::vector<VkDescriptorSetLayout>        descriptors(layoutParamsCount);
    std::vector<VkDescriptorSetLayoutBinding> bindingInfos;
    for (size_t i = 0; i < layoutParamsCount; ++i)
    {
        const auto& descriptorParams = layoutParams[i];
        for (const auto& binding : descriptorParams.bindings)
        {
            bindingInfos.emplace_back();
            auto& bindingInfo              = bindingInfos.back();
            bindingInfo.binding            = binding.index;
            bindingInfo.descriptorCount    = 1;
            bindingInfo.descriptorType     = (VkDescriptorType)binding.type;
            bindingInfo.stageFlags         = (VkShaderStageFlags)binding.stageFlags;
            bindingInfo.pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount                    = bindingInfos.size();
        layoutInfo.pBindings                       = bindingInfos.data();

        auto& descriptorSetLayout = descriptors[i];
        RI_CHECK_RESULT_MSG("couldn't create descriptor set layouts") =
            vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &descriptorSetLayout);
    }

    m_descriptorLayouts.insert(m_descriptorLayouts.end(), descriptors.begin(), descriptors.end());
    return startIndex;
}

}  // namespace ri
