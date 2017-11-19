
#include <ri/CommandBuffer.h>

#include "ri_internal_get_handle.h"

namespace ri
{
CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, bool isPrimary)
{
    static_assert(offsetof(ri::CommandBuffer, m_handle) == offsetof(ri::detail::CommandBuffer, m_handle),
                  "INVALID_CLASS_LAYOUT");

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = commandPool;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    RI_CHECK_RESULT() = vkAllocateCommandBuffers(device, &allocInfo, &m_handle);
}
}  // namespace ri
