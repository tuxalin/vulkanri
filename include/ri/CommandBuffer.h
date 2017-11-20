#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer : util::noncopyable, public detail::RenderObject<VkCommandBuffer>
{
public:
    void begin(RecordFlags flags);
    void end();
    void draw(uint32_t vertexCount, uint32_t instanceCount,  //
              uint32_t offsetVertexIndex = 0, uint32_t offsetInstanceIndex = 0);

private:
    CommandBuffer() {}
    CommandBuffer(VkDevice device, VkCommandPool commandPool, bool isPrimary);

private:
    friend class CommandPool;  // command buffers can only be constructed from a pool
};

inline CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, bool isPrimary)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = commandPool;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    RI_CHECK_RESULT() = vkAllocateCommandBuffers(device, &allocInfo, &m_handle);
}

inline void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,  //
                                uint32_t offsetVertexIndex, uint32_t offsetInstanceIndex)
{
    vkCmdDraw(m_handle, vertexCount, instanceCount, offsetVertexIndex, offsetInstanceIndex);
}

inline void CommandBuffer::begin(RecordFlags flags)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = (VkCommandBufferUsageFlags)flags;
    beginInfo.pInheritanceInfo         = nullptr;

    vkBeginCommandBuffer(m_handle, &beginInfo);
}

inline void CommandBuffer::end()
{
    RI_CHECK_RESULT() = vkEndCommandBuffer(m_handle);
}

}  // namespace ri
