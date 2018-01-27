#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;
class CommandPool;

class Buffer : util::noncopyable, public RenderObject<VkBuffer>
{
public:
    Buffer(const DeviceContext& device, int flags, size_t size);
    ~Buffer();

    size_t           bytes() const;
    BufferUsageFlags bufferUsage() const;

    void* lock();
    void* lock(size_t offset, size_t size);
    void* lock(size_t offset);
    void  unlock();
    void  update(const void* src);

    /// Copy from a staging buffer, issues an one time command submit, does this synchronously.
    void copy(const Buffer& src, CommandPool& commandPool, size_t srcOffset = 0, size_t dstOffset = 0);
    void copy(const Buffer& src, CommandPool& commandPool, size_t size, size_t srcOffset = 0, size_t dstOffset = 0);
    /// Copy from a staging buffer and issue a transfer command on the given command buffer.
    /// @note It's done asynchronously.
    void copy(const Buffer& src, CommandBuffer& commandBuffer, size_t srcOffset = 0, size_t dstOffset = 0);
    void copy(const Buffer& src, CommandBuffer& commandBuffer, size_t size, size_t srcOffset = 0, size_t dstOffset = 0);

private:
    uint32_t findMemoryIndex(const VkPhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter,
                             VkMemoryPropertyFlags properties);

    void allocateMemory(VkMemoryPropertyFlags flags);

private:
    VkDevice             m_device;
    const DeviceContext* m_deviceContext;
    VkDeviceMemory       m_bufferMemory;
    BufferUsageFlags     m_usage;
    size_t               m_size;
};

inline size_t Buffer::bytes() const
{
    return m_size;
}

inline BufferUsageFlags Buffer::bufferUsage() const
{
    return m_usage;
}

inline void* Buffer::lock()
{
    assert((m_usage.get() & BufferUsageFlags::eDst) == false);
    void* data;
    vkMapMemory(m_device, m_bufferMemory, 0, m_size, 0, &data);
    return data;
}

inline void* Buffer::lock(size_t offset, size_t size)
{
    assert((m_usage.get() & BufferUsageFlags::eDst) == false);
    assert((offset + size) <= m_size);
    void* data;
    vkMapMemory(m_device, m_bufferMemory, offset, m_size, 0, &data);
    return data;
}

inline void* Buffer::lock(size_t offset)
{
    assert((m_usage.get() & BufferUsageFlags::eDst) == false);
    assert(offset < m_size);
    void* data;
    vkMapMemory(m_device, m_bufferMemory, offset, VK_WHOLE_SIZE, 0, &data);
    return data;
}

inline void Buffer::unlock()
{
    vkUnmapMemory(m_device, m_bufferMemory);
}

inline void Buffer::update(const void* src)
{
    void* data = lock();
    memcpy(data, src, m_size);
    unlock();
}

inline void Buffer::copy(const Buffer& src, CommandPool& commandPool,  //
                         size_t srcOffset /*= 0*/, size_t dstOffset /*= 0*/)
{
    copy(src, commandPool, m_size, srcOffset, dstOffset);
}

inline void Buffer::copy(const Buffer& src, CommandBuffer& commandBuffer,  //
                         size_t srcOffset                                  /*= 0*/
                         ,
                         size_t dstOffset /*= 0*/)
{
    copy(src, commandBuffer, m_size, srcOffset, dstOffset);
}
}  // namespace ri
