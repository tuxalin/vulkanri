#pragma once

#include <vector>
#include <ri/Buffer.h>
#include <ri/Types.h>

namespace ri
{
class InputLayout
{
public:
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

    InputLayout();
    InputLayout(const VertexBinding& binding);
    InputLayout(const std::vector<VertexBinding>& bindings);
    InputLayout(const VertexBinding* bindings, size_t count);

    bool empty() const;

    void create(const VertexBinding& binding);
    void create(const std::vector<VertexBinding>& bindings);
    void create(const VertexBinding* bindings, size_t count);

    void bind(CommandBuffer& buffer) const;

private:
    void addAttributes(uint32_t binding, const std::vector<VertexInput>& attributes);

private:
    std::vector<VkBuffer>     m_vertexBuffers;
    std::vector<VkDeviceSize> m_vertexBufferOffsets;

    std::vector<VkVertexInputBindingDescription>   m_vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributeDescriptons;

    friend const std::vector<VkVertexInputBindingDescription>& detail::getLayoutBindingDescriptions(
        const InputLayout& layout);
    friend const std::vector<VkVertexInputAttributeDescription>& detail::getLayoutAttributeDescriptons(
        const InputLayout& layout);
};  // class InputLayout

class IndexedInputLayout : public InputLayout
{
public:
    using InputLayout::VertexBinding;
    using InputLayout::VertexInput;

    IndexedInputLayout();
    IndexedInputLayout(const VertexBinding& binding,  //
                       const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);
    IndexedInputLayout(const std::vector<VertexBinding>& bindings,  //
                       const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);
    IndexedInputLayout(const VertexBinding* bindings, size_t count,  //
                       const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);

    void bind(CommandBuffer& buffer);

    void setIndexBuffer(const Buffer& indexBuffer, IndexType indexType = IndexType::eInt16, uint32_t offset = 0);

private:
    VkBuffer     m_indexBuffer = VK_NULL_HANDLE;
    IndexType    m_indexType;
    VkDeviceSize m_offset = 0;
#ifndef NDEBUG
    uint32_t m_size = 0;
#endif  // !NDEBUG

};  // class IndexedInputLayout

inline InputLayout::InputLayout() {}

inline InputLayout::InputLayout(const VertexBinding& binding)
{
    create(&binding, 1);
}

inline InputLayout::InputLayout(const std::vector<VertexBinding>& bindings)
{
    create(bindings.data(), bindings.size());
}

inline InputLayout::InputLayout(const VertexBinding* bindings, size_t count)
{
    create(bindings, count);
}

inline void InputLayout::create(const VertexBinding& binding)
{
    create(&binding, 1);
}

inline void InputLayout::create(const std::vector<VertexBinding>& bindings)
{
    create(bindings.data(), bindings.size());
}

inline void InputLayout::bind(CommandBuffer& buffer) const
{
    assert(!empty());
    vkCmdBindVertexBuffers(detail::getVkHandle(buffer), 0, m_vertexBuffers.size(), m_vertexBuffers.data(),
                           m_vertexBufferOffsets.data());
}

inline bool InputLayout::empty() const
{
    return m_vertexBindingDescriptions.empty();
}

//

inline IndexedInputLayout::IndexedInputLayout() {}

inline IndexedInputLayout::IndexedInputLayout(const VertexBinding& binding,  //
                                              const Buffer& indexBuffer, IndexType indexType, uint32_t offset)
    : InputLayout(binding)
    , m_indexBuffer(detail::getVkHandle(indexBuffer))
    , m_indexType(indexType)
    , m_offset(offset)
#ifndef NDEBUG
    , m_size(indexBuffer.bytes() / (m_indexType == IndexType::eInt16 ? 2 : 4))
#endif
{
}

inline IndexedInputLayout::IndexedInputLayout(const std::vector<VertexBinding>& bindings,  //
                                              const Buffer& indexBuffer, IndexType indexType, uint32_t offset)
    : InputLayout(bindings)
    , m_indexBuffer(detail::getVkHandle(indexBuffer))
    , m_indexType(indexType)
    , m_offset(offset)
#ifndef NDEBUG
    , m_size(indexBuffer.bytes() / (m_indexType == IndexType::eInt16 ? 2 : 4))
#endif
{
}

inline IndexedInputLayout::IndexedInputLayout(const VertexBinding* bindings, size_t count,  //
                                              const Buffer& indexBuffer, IndexType indexType, uint32_t offset)
    : InputLayout(bindings, count)
    , m_indexBuffer(detail::getVkHandle(indexBuffer))
    , m_indexType(indexType)
    , m_offset(offset)
#ifndef NDEBUG
    , m_size(indexBuffer.bytes() / (m_indexType == IndexType::eInt16 ? 2 : 4))
#endif
{
}

inline void IndexedInputLayout::bind(CommandBuffer& buffer)
{
    assert(m_indexBuffer);
    assert(m_offset < m_size);
    InputLayout::bind(buffer);
    vkCmdBindIndexBuffer(detail::getVkHandle(buffer), m_indexBuffer, m_offset, (VkIndexType)m_indexType);
}

inline void IndexedInputLayout::setIndexBuffer(const Buffer& buffer, IndexType indexType, uint32_t offset)
{
    assert(buffer.bufferUsage().get() & BufferUsageFlags::eIndex);
    m_indexBuffer = detail::getVkHandle(buffer);
    m_indexType   = indexType;
    m_offset      = offset;
#ifndef NDEBUG
    m_size = buffer.bytes() / (m_indexType == IndexType::eInt16 ? 2 : 4);
#endif
}

namespace detail
{
    inline const std::vector<VkVertexInputBindingDescription>& getLayoutBindingDescriptions(const InputLayout& layout)
    {
        return layout.m_vertexBindingDescriptions;
    }
    inline const std::vector<VkVertexInputAttributeDescription>& getLayoutAttributeDescriptons(
        const InputLayout& layout)
    {
        return layout.m_vertexAttributeDescriptons;
    }
}  // namespace detail
}  // namespace ri
