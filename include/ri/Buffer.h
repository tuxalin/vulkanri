#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class Buffer : util::noncopyable, public detail::RenderObject<VkBuffer>
{
public:
    Buffer(const DeviceContext& device, BufferUsageFlags flags, size_t size);
    ~Buffer();

    size_t           bytes() const;
    BufferUsageFlags bufferUsage() const;

    void* lock();
    void* lock(size_t offset, size_t size);
    void* lock(size_t offset);
    void  unlock();
    void  update(const void* src);

private:
    uint32_t findMemoryIndex(const VkPhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter,
                             VkMemoryPropertyFlags properties);

    void allocateMemory(const DeviceContext& device);

private:
    VkDevice         m_device;
    VkDeviceMemory   m_bufferMemory;
    BufferUsageFlags m_flags;
    size_t           m_size;
};

inline size_t Buffer::bytes() const
{
    return m_size;
}

inline BufferUsageFlags Buffer::bufferUsage() const
{
    return m_flags;
}

inline void* Buffer::lock()
{
    void* data;
    vkMapMemory(m_device, m_bufferMemory, 0, m_size, 0, &data);
    return data;
}

inline void* Buffer::lock(size_t offset, size_t size)
{
    assert((offset + size) < m_size);
    void* data;
    vkMapMemory(m_device, m_bufferMemory, offset, m_size, 0, &data);
    return data;
}

inline void* Buffer::lock(size_t offset)
{
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

}  // namespace ri
