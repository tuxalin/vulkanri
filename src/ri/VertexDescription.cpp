
#include <ri/VertexDescription.h>

namespace ri
{
void VertexDescription::create(const VertexBinding* bindings, size_t count)
{
    static_assert(static_cast<VkVertexInputRate>(true) == VK_VERTEX_INPUT_RATE_INSTANCE, "");

    m_vertexAttributeDescriptons.clear();
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

void VertexDescription::addAttributes(uint32_t binding, const std::vector<VertexInput>& attributes)
{
    assert(!attributes.empty());

    for (const auto& input : attributes)
    {
        m_vertexAttributeDescriptons.emplace_back();
        auto& attributeDescription    = m_vertexAttributeDescriptons.back();
        attributeDescription.binding  = binding;
        attributeDescription.location = input.location;
        attributeDescription.format   = (VkFormat)input.format;
        attributeDescription.offset   = input.offset;
    }
}
}  // namespace ri
