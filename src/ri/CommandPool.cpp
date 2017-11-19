
#include <ri/CommandPool.h>

#include <algorithm>
#include <ri/CommandBuffer.h>

namespace ri
{
CommandPool::CommandPool(DeviceCommandHint commandHint)
    : m_commandHint(commandHint)
{
}

CommandPool::~CommandPool()
{
    if (m_device)
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

CommandBuffer* CommandPool::create(bool isPrimary /*= true*/)
{
    assert(m_device && m_commandPool);

    return new CommandBuffer(m_device, m_commandPool, isPrimary);
}

void CommandPool::create(std::vector<CommandBuffer*>& buffers, bool isPrimary /*= true*/)
{
    assert(m_device && m_commandPool);
    assert(!buffers.empty());

    std::vector<VkCommandBuffer> bufferHandles(buffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_commandPool;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = bufferHandles.size();

    RI_CHECK_RESULT() = vkAllocateCommandBuffers(m_device, &allocInfo, bufferHandles.data());

    std::transform(bufferHandles.begin(), bufferHandles.end(), buffers.begin(), [](auto handle) -> CommandBuffer* {
        assert(handle);
        auto buffer      = new CommandBuffer();
        buffer->m_handle = handle;
        return buffer;
    });
}

void CommandPool::initialize(VkDevice device, int queueIndex)
{
    assert(device);

    m_device = device;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex        = queueIndex;
    poolInfo.flags                   = (VkCommandPoolCreateFlags)m_commandHint;

    auto res = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    assert(!res);
}
}  // namespace ri
