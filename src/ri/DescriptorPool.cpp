
#include <ri/DescriptorPool.h>
#include <ri/DescriptorSet.h>

namespace ri
{
DescriptorPool::DescriptorPool(const DeviceContext& device, DescriptorType type, size_t maxCount)
    : m_device(ri::detail::getVkHandle(device))
    , m_type(type)
{
    VkDescriptorPoolSize poolSize = {};
    poolSize.type                 = (VkDescriptorType)type;
    poolSize.descriptorCount      = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount              = 1;
    poolInfo.pPoolSizes                 = &poolSize;
    poolInfo.maxSets                    = maxCount;
    poolInfo.flags                      = 0;

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

DescriptorSet DescriptorPool::create(const DescriptorSetParams& params, size_t layoutIndex)
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

    return DescriptorSet(m_device, handle, m_type, params);
}

void DescriptorPool::create(const std::vector<DescriptorSetParams>& descriptorParams,  //
                            const std::vector<size_t>&              layoutIndices,     //
                            std::vector<DescriptorSet>&             descriptors)
{
    assert(descriptorParams.size() == layoutIndices.size());

    std::vector<VkDescriptorSetLayout> layouts(descriptorParams.size());
    for (size_t i = 0; i < descriptorParams.size(); ++i)
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

    std::vector<VkDescriptorSet> handles(descriptorParams.size());
    RI_CHECK_RESULT_MSG("couldn't allocate descriptor sets") =
        vkAllocateDescriptorSets(m_device, &allocInfo, handles.data());

    descriptors.clear();
    descriptors.reserve(descriptorParams.size());
    for (size_t i = 0; i < descriptorParams.size(); ++i)
    {
        descriptors.push_back(DescriptorSet(m_device, handles[i], m_type, descriptorParams[i]));
    }
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
        bindingInfo.descriptorType     = (VkDescriptorType)binding.descriptorType;
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

size_t DescriptorPool::createLayouts(const std::vector<DescriptorLayoutParam>& layoutParams)
{
    size_t      index   = m_descriptorLayouts.size();
    const auto& layouts = createDescriptorLayouts(layoutParams);
    m_descriptorLayouts.insert(m_descriptorLayouts.end(), layouts.begin(), layouts.end());
    return index;
}

std::vector<VkDescriptorSetLayout> DescriptorPool::createDescriptorLayouts(
    const std::vector<DescriptorLayoutParam>& layoutParams)
{
    std::vector<VkDescriptorSetLayout> descriptors(layoutParams.size());

    std::vector<VkDescriptorSetLayoutBinding> bindingInfos;
    for (size_t i = 0; i < layoutParams.size(); ++i)
    {
        const auto& descriptorParams = layoutParams[i];
        for (const auto& binding : descriptorParams.bindings)
        {
            bindingInfos.emplace_back();
            auto& bindingInfo              = bindingInfos.back();
            bindingInfo.binding            = binding.index;
            bindingInfo.descriptorCount    = 1;
            bindingInfo.descriptorType     = (VkDescriptorType)binding.descriptorType;
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
    return descriptors;
}

}  // namespace ri
