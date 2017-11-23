
#include <ri/Buffer.h>

#include <ri/CommandBuffer.h>
#include <ri/CommandPool.h>
#include <ri/DeviceContext.h>

namespace ri
{
Buffer::Buffer(const DeviceContext& device, int flags, size_t size)
    : m_device(detail::getVkHandle(device))
    , m_deviceContext(&device)
    , m_usage(flags)
    , m_size(size)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = m_size;
    bufferInfo.usage              = (VkBufferUsageFlags)flags;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    RI_CHECK_RESULT() = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_handle);

    const VkMemoryPropertyFlags kStagingFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocateMemory((m_usage.get() & BufferUsageFlags::eSrc) ? kStagingFlags : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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

inline void Buffer::allocateMemory(VkMemoryPropertyFlags flags)
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;

    const VkPhysicalDeviceMemoryProperties& memProperties = detail::getDeviceMemoryProperties(*m_deviceContext);
    allocInfo.memoryTypeIndex = findMemoryIndex(memProperties, memRequirements.memoryTypeBits, flags);

    RI_CHECK_RESULT() = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_bufferMemory);

    RI_CHECK_RESULT() = vkBindBufferMemory(m_device, m_handle, m_bufferMemory, 0);
}

void Buffer::copy(const Buffer& src, CommandPool& commandPool, size_t size, size_t srcOffset /*= 0*/,
                  size_t dstOffset /*= 0*/)
{
    CommandBuffer* commandBuffer = commandPool.create();

    commandBuffer->begin(RecordFlags::eOneTime);
    copy(src, *commandBuffer, size, srcOffset, dstOffset);
    commandBuffer->end();

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    const auto handle             = detail::getVkHandle(*commandBuffer);
    submitInfo.pCommandBuffers    = &handle;

    auto transferQueue = detail::getDeviceQueue(*m_deviceContext, DeviceOperation::eTransfer);
    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue);

    delete commandBuffer;
}

void Buffer::copy(const Buffer& src, CommandBuffer& commandBuffer, size_t size,  //
                  size_t srcOffset /*= 0*/, size_t dstOffset /*= 0*/)
{
    assert(src.bufferUsage().get() & BufferUsageFlags::eSrc);
    assert((src.bytes() - srcOffset) >= (size - dstOffset));
    assert(m_size >= size);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset    = srcOffset;
    copyRegion.dstOffset    = dstOffset;
    copyRegion.size         = size;

    vkCmdCopyBuffer(detail::getVkHandle(commandBuffer), src.m_handle, m_handle, 1, &copyRegion);
}

}  // namespace ri
