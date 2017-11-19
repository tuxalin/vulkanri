
#include <ri/CommandBuffer.h>

namespace ri
{
CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, bool isPrimary)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = commandPool;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    RI_CHECK_RESULT() = vkAllocateCommandBuffers(device, &allocInfo, &m_handle);
}
}  // namespace ri
