#pragma once

#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/RenderPipeline.h>
#include <ri/Types.h>

namespace ri
{
struct DescriptorSetParams
{
    const Buffer* buffer;
    size_t        offset, size;
};

class DescriptorSet : public RenderObject<VkDescriptorSet>
{
public:
    DescriptorSet();

    void update(const DescriptorSetParams& params);
    void update(const Buffer& buffer, size_t offset, size_t size);
    void setBindPoint(bool isGraphics);

    void bind(CommandBuffer& buffer, const RenderPipeline& pipeline);

    DescriptorType type() const;

    /// Batch call for setting multiple descriptors.
    ///@note Preferred over individual calls.
    template <int Count>
    static void update(const DescriptorSet* (&descriptors)[Count],
                       const DescriptorSetParams (&descriptorParams)[Count]);
    /// Batch call for binding multiple descriptors.
    ///@note Preferred over individual calls.
    template <int Count>
    static void bind(CommandBuffer& buffer, const RenderPipeline& pipeline, const DescriptorSet* (&descriptors)[Count]);

private:
    DescriptorSet(VkDevice device, VkDescriptorSet handle, DescriptorType type, const DescriptorSetParams& params)
        : RenderObject<VkDescriptorSet>(handle)
        , m_device(device)
        , m_type(type)
    {
        update(params);
    }

    VkDevice            m_device = VK_NULL_HANDLE;
    DescriptorType      m_type;
    VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    friend class DescriptorPool;  // DescriptorSet can be created only from a pool
};

inline DescriptorSet::DescriptorSet() {}

inline void DescriptorSet::update(const DescriptorSetParams& params)
{
    assert(params.buffer);
    update(*params.buffer, params.offset, params.size);
}

inline void DescriptorSet::update(const Buffer& buffer, size_t offset, size_t size)
{
    assert(m_handle);

    const DescriptorSetParams params[]     = {{&buffer, offset, size}};
    const DescriptorSet*      descriptor[] = {this};
    update<1>(descriptor, params);
}

inline void DescriptorSet::setBindPoint(bool isGraphics)
{
    assert(m_handle);
    m_bindPoint = isGraphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
}

inline void DescriptorSet::bind(CommandBuffer& buffer, const RenderPipeline& pipeline)
{
    assert(m_handle);
    vkCmdBindDescriptorSets(detail::getVkHandle(buffer), m_bindPoint, detail::getPipelineLayout(pipeline), 0, 1,
                            &m_handle, 0, nullptr);
}

inline DescriptorType DescriptorSet::type() const
{
    return m_type;
}

template <int Count>
static void DescriptorSet::update(const DescriptorSet* (&descriptors)[Count],
                                  const DescriptorSetParams (&descriptorParams)[Count])
{
    VkDescriptorBufferInfo bufferInfos[Count];
    VkWriteDescriptorSet   descriptorWriteInfos[Count];

    const auto device = descriptors[0]->m_device;
    for (int i = 0; i < Count; ++i)
    {
        auto descriptor = descriptors[i];
        assert(descriptor);
        assert(descriptor->m_handle);

        const auto& params     = descriptorParams[i];
        auto&       bufferInfo = bufferInfos[i];
        bufferInfo.buffer      = detail::getVkHandle(*params.buffer);
        bufferInfo.offset      = params.offset;
        bufferInfo.range       = params.size;

        auto& descriptorWrite  = descriptorWriteInfos[i];
        descriptorWrite.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptor->m_handle;
        // TODO: expose array update
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = (VkDescriptorType)descriptor->m_type;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &bufferInfo;
        descriptorWrite.pImageInfo       = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;
        descriptorWrite.pNext            = nullptr;
    }
    vkUpdateDescriptorSets(device, Count, descriptorWriteInfos, 0, nullptr);
}

template <int Count>
void DescriptorSet::bind(CommandBuffer& buffer, const RenderPipeline& pipeline,
                         const DescriptorSet* (&descriptors)[Count])
{
    VkDescriptorSet handles[Count];

    for (int i = 0; i < Count; ++i)
    {
        auto descriptor = descriptors[i];
        assert(descriptor);
        assert(descriptor->m_handle);
        handles[i] = descriptor->m_handle;
    }

    vkCmdBindDescriptorSets(detail::getVkHandle(buffer), m_bindPoint, detail::getPipelineLayout(pipeline), 0, Count,
                            handles, 0, nullptr);
}

}  // namespace ri
