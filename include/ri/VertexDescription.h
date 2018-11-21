#pragma once

#include <vector>
#include <ri/Buffer.h>
#include <ri/Types.h>

namespace ri
{
struct VertexInput
{
    uint32_t        location;
    AttributeFormat format;
    uint32_t        offset;
};

struct VertexBinding
{
    uint32_t                 bindingIndex;
    uint32_t                 stride;
    uint32_t                 offset;
    bool                     instanced = false;
    Buffer*                  buffer    = nullptr;
    std::vector<VertexInput> attributes;

    VertexBinding() {}
    VertexBinding(std::initializer_list<VertexInput> attributes)
        : attributes(attributes)
    {
    }
};

class VertexDescription : public TagableObject
{
public:
    VertexDescription();
    VertexDescription(const VertexBinding& binding);
    VertexDescription(const VertexBinding* bindings, size_t count);
    VertexDescription(const std::vector<VertexBinding>& bindings);

    bool empty() const;

    void create(const VertexBinding& binding);
    void create(const VertexBinding* bindings, size_t count);
    void create(const std::vector<VertexBinding>& bindings);

    void bind(CommandBuffer& buffer) const;

private:
    void addAttributes(uint32_t binding, const std::vector<VertexInput>& attributes);

private:
    std::vector<VkBuffer>     m_vertexBuffers;
    std::vector<VkDeviceSize> m_vertexBufferOffsets;

    std::vector<VkVertexInputBindingDescription>   m_vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributeDescriptons;

    friend const std::vector<VkVertexInputBindingDescription>& detail::getBindingDescriptions(
        const VertexDescription& layout);
    friend const std::vector<VkVertexInputAttributeDescription>& detail::getAttributeDescriptons(
        const VertexDescription& layout);
};  // class InputLayout

class IndexedVertexDescription : public VertexDescription
{
public:
    IndexedVertexDescription();
    IndexedVertexDescription(const VertexBinding& binding,  //
                             const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);
    IndexedVertexDescription(const VertexBinding* bindings, size_t count,  //
                             const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);
    IndexedVertexDescription(const std::vector<VertexBinding>& bindings,  //
                             const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);

    void bind(CommandBuffer& buffer) const;

    void setIndexBuffer(const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0,
                        uint32_t count = 0);

    uint32_t count() const;

private:
    VkBuffer     m_indexBuffer = VK_NULL_HANDLE;
    IndexType    m_indexType;
    VkDeviceSize m_offset = 0;
    uint32_t     m_count  = 0;
#ifndef NDEBUG
    uint32_t m_bufferSize = 0;
#endif  // !NDEBUG

};  // class IndexedInputLayout

inline VertexDescription::VertexDescription() {}

inline VertexDescription::VertexDescription(const VertexBinding& binding)
{
    create(&binding, 1);
}
inline VertexDescription::VertexDescription(const VertexBinding* bindings, size_t count)
{
    create(bindings, count);
}

inline VertexDescription::VertexDescription(const std::vector<VertexBinding>& bindings)
{
    create(bindings.data(), bindings.size());
}

inline void VertexDescription::create(const VertexBinding& binding)
{
    create(&binding, 1);
}

inline void VertexDescription::create(const std::vector<VertexBinding>& bindings)
{
    create(bindings.data(), bindings.size());
}

inline void VertexDescription::bind(CommandBuffer& buffer) const
{
    assert(!empty());
    vkCmdBindVertexBuffers(detail::getVkHandle(buffer), 0, m_vertexBuffers.size(), m_vertexBuffers.data(),
                           m_vertexBufferOffsets.data());
}

inline bool VertexDescription::empty() const
{
    return m_vertexBindingDescriptions.empty();
}

//

inline IndexedVertexDescription::IndexedVertexDescription() {}

inline IndexedVertexDescription::IndexedVertexDescription(const VertexBinding& binding,  //
                                                          const Buffer& indexBuffer, IndexType indexType,
                                                          uint32_t offset)
    : VertexDescription(binding)
    , m_indexBuffer(detail::getVkHandle(indexBuffer))
    , m_indexType(indexType)
    , m_offset(offset)
#ifndef NDEBUG
    , m_bufferSize(indexBuffer.bytes())
#endif
{
    const uint32_t elementSize = m_indexType == IndexType::eInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
    m_count                    = indexBuffer.bytes() / elementSize;
}

inline IndexedVertexDescription::IndexedVertexDescription(const VertexBinding* bindings, size_t count,  //
                                                          const Buffer& indexBuffer, IndexType indexType,
                                                          uint32_t offset)
    : VertexDescription(bindings, count)
    , m_indexBuffer(detail::getVkHandle(indexBuffer))
    , m_indexType(indexType)
    , m_offset(offset)
#ifndef NDEBUG
    , m_bufferSize(indexBuffer.bytes())
#endif
{
    const uint32_t elementSize = m_indexType == IndexType::eInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
    m_count                    = indexBuffer.bytes() / elementSize;
}

inline IndexedVertexDescription::IndexedVertexDescription(const std::vector<VertexBinding>& bindings,  //
                                                          const Buffer& indexBuffer, IndexType indexType,
                                                          uint32_t offset)
    : VertexDescription(bindings)
    , m_indexBuffer(detail::getVkHandle(indexBuffer))
    , m_indexType(indexType)
    , m_offset(offset)
#ifndef NDEBUG
    , m_bufferSize(indexBuffer.bytes())
#endif
{
    const uint32_t elementSize = m_indexType == IndexType::eInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
    m_count                    = indexBuffer.bytes() / elementSize;
}

inline void IndexedVertexDescription::bind(CommandBuffer& buffer) const
{
    assert(m_indexBuffer);
    assert((m_offset + m_count * (m_indexType == IndexType::eInt16 ? sizeof(uint16_t) : sizeof(uint32_t))) <=
           m_bufferSize);
    VertexDescription::bind(buffer);
    vkCmdBindIndexBuffer(detail::getVkHandle(buffer), m_indexBuffer, m_offset, (VkIndexType)m_indexType);
}

inline void IndexedVertexDescription::setIndexBuffer(const Buffer& buffer, IndexType indexType, uint32_t offset /*= 0*/,
                                                     uint32_t count /*= 0*/)
{
    assert(buffer.bufferUsage().get() & BufferUsageFlags::eIndex);
    m_indexBuffer = detail::getVkHandle(buffer);
    m_indexType   = indexType;
    m_offset      = offset;

    const uint32_t elementSize = m_indexType == IndexType::eInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
    m_count                    = count == 0 ? buffer.bytes() / elementSize : count;

#ifndef NDEBUG
    m_bufferSize = buffer.bytes();
#endif
    assert((m_offset + count * elementSize) <= m_bufferSize);
}

inline uint32_t IndexedVertexDescription::count() const
{
    return m_count;
}

namespace detail
{
    inline const std::vector<VkVertexInputBindingDescription>& getBindingDescriptions(  //
        const VertexDescription& layout)
    {
        return layout.m_vertexBindingDescriptions;
    }
    inline const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptons(  //
        const VertexDescription& layout)
    {
        return layout.m_vertexAttributeDescriptons;
    }
}  // namespace detail
}  // namespace ri
