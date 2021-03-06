
#include <ri/CommandPool.h>

#include <algorithm>
#include <ri/CommandBuffer.h>

namespace ri
{
CommandPool::CommandPool(bool resetMode, DeviceCommandHint commandHint, DeviceOperation deviceOp)
    : m_commandHint(commandHint)
    , m_deviceOp(deviceOp)
    , m_resetMode(resetMode)
{
    m_oneTimeBuffers.reserve(10);
}

CommandPool::~CommandPool()
{
    if (m_device)
        vkDestroyCommandPool(detail::getVkHandle(*m_device), m_handle, nullptr);
}

CommandBuffer CommandPool::create(bool isPrimary /*= true*/)
{
    assert(m_device && m_handle);
    return CommandBuffer(detail::getVkHandle(*m_device), m_handle, isPrimary);
}

void CommandPool::create(CommandBuffer* buffers, size_t buffersCount, bool isPrimary /*= true*/)
{
    assert(m_device && m_handle);
    assert(buffers);

    std::vector<VkCommandBuffer> bufferHandles(buffersCount);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_handle;
    allocInfo.level              = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = bufferHandles.size();

    RI_CHECK_RESULT_MSG("error at allocating multiple buffers") =
        vkAllocateCommandBuffers(detail::getVkHandle(*m_device), &allocInfo, bufferHandles.data());

    std::transform(bufferHandles.begin(), bufferHandles.end(), buffers,
                   [this, isPrimary](auto handle) -> CommandBuffer {
                       assert(handle);
                       return CommandBuffer(detail::getVkHandle(*m_device), m_handle, handle);
                   });
}

void CommandPool::create(std::vector<CommandBuffer>& buffers, bool isPrimary /*= true*/)
{
    assert(!buffers.empty());
    create(buffers.data(), buffers.size());
}

CommandBuffer CommandPool::begin()
{
    CommandBuffer commandBuffer(detail::getVkHandle(*m_device), m_handle, true);
    commandBuffer.begin(RecordFlags::eOneTime);

    return commandBuffer;
}

void CommandPool::end(CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer.m_handle;
    const auto queueHandle        = detail::getDeviceQueue(*m_device, (int)m_deviceOp);
    RI_CHECK_RESULT_MSG("error at command pool end buffer") =
        vkQueueSubmit(queueHandle, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queueHandle);

    commandBuffer.destroy();
}

void CommandPool::free(CommandBuffer* buffers, size_t buffersCount)
{
    std::vector<VkCommandBuffer> bufferHandles(buffersCount);
    std::transform(buffers, buffers + buffersCount, bufferHandles.begin(),
                   [](const CommandBuffer& buffer) -> VkCommandBuffer { return buffer.m_handle; });

    vkFreeCommandBuffers(detail::getVkHandle(*m_device), m_handle, bufferHandles.size(), bufferHandles.data());
    std::for_each(buffers, buffers + buffersCount, [](CommandBuffer& buffer) { buffer.m_handle = VK_NULL_HANDLE; });
}

void CommandPool::free(std::vector<CommandBuffer>& buffers)
{
    free(buffers.data(), buffers.size());
}

void CommandPool::initialize(const DeviceContext& device, int queueIndex)
{
    m_device = &device;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex        = queueIndex;
    VkCommandPoolCreateFlags flags   = (VkCommandPoolCreateFlags)m_commandHint;
    if (m_resetMode)
        flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.flags = flags;

    RI_CHECK_RESULT_MSG("couldn't create command pool") =
        vkCreateCommandPool(detail::getVkHandle(*m_device), &poolInfo, nullptr, &m_handle);
}
}  // namespace ri
