
#include <ri/Buffer.h>

#include <ri/DeviceContext.h>

namespace ri
{
Buffer::Buffer(const DeviceContext& device, BufferUsageFlags flags, size_t size)
    : m_device(detail::getVkHandle(device))
    , m_flags(flags)
    , m_size(size)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = m_size;
    bufferInfo.usage              = (VkBufferUsageFlags)flags;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    RI_CHECK_RESULT() = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_handle);

    allocateMemory(device);
}

Buffer::~Buffer()
{
    vkDestroyBuffer(m_device, m_handle, nullptr);
    vkFreeMemory(m_device, m_bufferMemory, nullptr);
}

inline uint32_t Buffer::findMemoryIndex(const VkPhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter,
                                        VkMemoryPropertyFlags flags)
{
    size_t i     = 0;
    auto   found = std::find_if(memProperties.memoryTypes, memProperties.memoryTypes + memProperties.memoryTypeCount,
                              [typeFilter, flags, &i](const auto& memoryType) {
                                  return (typeFilter & (1 << i++)) && (memoryType.propertyFlags & flags) == flags;
                              });
    assert(found != (memProperties.memoryTypes + memProperties.memoryTypeCount));
    return i - 1;
}

inline void Buffer::allocateMemory(const DeviceContext& device)
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;

    const VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkPhysicalDeviceMemoryProperties& memProperties = detail::getDeviceMemoryProperties(device);
    allocInfo.memoryTypeIndex = findMemoryIndex(memProperties, memRequirements.memoryTypeBits, flags);

    RI_CHECK_RESULT() = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_bufferMemory);

    RI_CHECK_RESULT() = vkBindBufferMemory(m_device, m_handle, m_bufferMemory, 0);
}

}  // namespace ri
