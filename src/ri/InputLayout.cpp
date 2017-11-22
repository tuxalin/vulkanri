
#include <ri/InputLayout.h>

namespace ri
{
void InputLayout::create(const VertexBinding* bindings, size_t count)
{
    static_assert(static_cast<VkVertexInputRate>(true) == VK_VERTEX_INPUT_RATE_INSTANCE, "");

    m_vertexBindingDescriptions.resize(count);
    m_vertexBuffers.resize(count);
    m_vertexBufferOffsets.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        const auto& binding            = bindings[i];
        auto&       bindingDescription = m_vertexBindingDescriptions[i];
        bindingDescription.binding     = binding.bindingIndex;
        bindingDescription.stride      = binding.stride;
        bindingDescription.inputRate   = (VkVertexInputRate)binding.instanced;

        assert(binding.buffer);
        m_vertexBuffers[i]       = detail::getVkHandle(*binding.buffer);
        m_vertexBufferOffsets[i] = binding.offset;

        addAttributes(binding.bindingIndex, binding.attributes);
    }
}

void InputLayout::addAttributes(uint32_t binding, const std::vector<VertexInput>& attributes)
{
    assert(!attributes.empty());

    m_vertexAttributeDescriptons.resize(attributes.size());

    size_t i = 0;
    for (const auto& input : attributes)
    {
        auto& attributeDescription    = m_vertexAttributeDescriptons[i++];
        attributeDescription.binding  = binding;
        attributeDescription.location = input.location;
        attributeDescription.format   = (VkFormat)input.format;
        attributeDescription.offset   = input.offset;
    }
}
}  // namespace ri
