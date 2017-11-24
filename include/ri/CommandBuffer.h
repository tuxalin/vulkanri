#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer : util::noncopyable, public RenderObject<VkCommandBuffer>
{
public:
    enum ResetFlags
    {
        // The command buffer may hold onto memory resources and reuse them when recording commands.
        ePreserve = 0,
        //  Specifies that most or all memory resources currently owned by the command buffer should be returned to the
        //  parent command pool.
        eRelease = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT
    };

    ~CommandBuffer();

    void begin(RecordFlags flags);
    void end();
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1,  //
              uint32_t offsetVertexIndex = 0, uint32_t offsetInstanceIndex = 0);
    void drawIndexed(uint32_t vertexCount, uint32_t instanceCount = 1,  //
                     uint32_t offsetIndex = 0, uint32_t offsetVertexIndex = 0, uint32_t offsetInstanceIndex = 0);

    ///@note Can only be used if the buffer was created from a pool with reset mode.
    void reset(ResetFlags flags = ePreserve);

private:
    CommandBuffer();
    CommandBuffer(VkDevice device, VkCommandPool commandPool, bool isPrimary);

private:
    VkCommandPool m_commandPool;
    VkDevice      m_device;

    friend class CommandPool;  // command buffers can only be constructed from a pool
};

inline CommandBuffer::CommandBuffer()
    : m_commandPool(VK_NULL_HANDLE)
    , m_device(VK_NULL_HANDLE)
{
}

inline CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, bool isPrimary)
    : m_commandPool(commandPool)
    , m_device(device)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = commandPool;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    RI_CHECK_RESULT() = vkAllocateCommandBuffers(device, &allocInfo, &m_handle);
}

inline CommandBuffer::~CommandBuffer()
{
    if (m_commandPool)
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_handle);
}

inline void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,  //
                                uint32_t offsetVertexIndex, uint32_t offsetInstanceIndex)
{
    vkCmdDraw(m_handle, vertexCount, instanceCount, offsetVertexIndex, offsetInstanceIndex);
}

inline void CommandBuffer::drawIndexed(uint32_t vertexCount, uint32_t instanceCount,  //
                                       uint32_t offsetIndex, uint32_t offsetVertexIndex, uint32_t offsetInstanceIndex)
{
    vkCmdDrawIndexed(m_handle, vertexCount, instanceCount, offsetIndex, offsetVertexIndex, offsetInstanceIndex);
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

inline void CommandBuffer::reset(ResetFlags flags /*= ePreserve*/)
{
    vkResetCommandBuffer(m_handle, flags);
}

}  // namespace ri
