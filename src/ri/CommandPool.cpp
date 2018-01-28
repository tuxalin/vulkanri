
#include <ri/CommandPool.h>

#include <algorithm>
#include <ri/CommandBuffer.h>

namespace ri
{
CommandPool::CommandPool(bool resetMode, DeviceCommandHint commandHint)
    : m_commandHint(commandHint)
    , m_resetMode(resetMode)
{
}

CommandPool::~CommandPool()
{
    if (m_device)
        vkDestroyCommandPool(m_device, m_handle, nullptr);
}

CommandBuffer* CommandPool::create(bool isPrimary /*= true*/)
{
    assert(m_device && m_handle);

    return new CommandBuffer(m_device, m_handle, isPrimary);
}

void CommandPool::create(std::vector<CommandBuffer*>& buffers, bool isPrimary /*= true*/)
{
    assert(m_device && m_handle);
    assert(!buffers.empty());

    std::vector<VkCommandBuffer> bufferHandles(buffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_handle;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = bufferHandles.size();

    RI_CHECK_RESULT_MSG("error at allocating multiple buffers") =
        vkAllocateCommandBuffers(m_device, &allocInfo, bufferHandles.data());

    std::transform(bufferHandles.begin(), bufferHandles.end(), buffers.begin(), [](auto handle) -> CommandBuffer* {
        assert(handle);
        return new CommandBuffer(handle);
    });
}

void CommandPool::free(std::vector<CommandBuffer*>& buffers)
{
    std::vector<VkCommandBuffer> bufferHandles(buffers.size());

    std::transform(buffers.begin(), buffers.end(), bufferHandles.begin(), [](auto buffer) -> VkCommandBuffer {
        assert(buffer);
        return buffer->m_handle;
    });

    for (auto& buffer : buffers)
    {
        assert(buffer);
        buffer->m_commandPool = VK_NULL_HANDLE;
        delete buffer;
        buffer = nullptr;
    }

    vkFreeCommandBuffers(m_device, m_handle, bufferHandles.size(), bufferHandles.data());
}

void CommandPool::initialize(VkDevice device, int queueIndex)
{
    assert(device);

    m_device = device;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex        = queueIndex;
    VkCommandPoolCreateFlags flags   = (VkCommandPoolCreateFlags)m_commandHint;
    if (m_resetMode)
        flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.flags = flags;

    RI_CHECK_RESULT_MSG("couldn't create command pool") = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_handle);
}
}  // namespace ri
