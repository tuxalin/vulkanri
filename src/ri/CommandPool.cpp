
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

    RI_CHECK_RESULT() = vkAllocateCommandBuffers(m_device, &allocInfo, bufferHandles.data());

    std::transform(bufferHandles.begin(), bufferHandles.end(), buffers.begin(), [](auto handle) -> CommandBuffer* {
        assert(handle);
        auto buffer      = new CommandBuffer();
        buffer->m_handle = handle;
        return buffer;
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
    poolInfo.flags                   = (VkCommandPoolCreateFlags)m_commandHint;

    RI_CHECK_RESULT() = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_handle);
}
}  // namespace ri
