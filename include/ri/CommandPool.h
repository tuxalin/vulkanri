#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;

class CommandPool : util::noncopyable, public detail::RenderObject<VkCommandPool>
{
public:
    CommandBuffer* create(bool isPrimary = true);
    void           create(std::vector<CommandBuffer*>& buffers, bool isPrimary = true);

    void free(std::vector<CommandBuffer*>& buffers);

private:
    CommandPool(DeviceCommandHint commandHint);
    ~CommandPool();

    void initialize(VkDevice device, int queueIndex);

private:
    VkDevice          m_device = VK_NULL_HANDLE;
    DeviceCommandHint m_commandHint;

    friend class DeviceContext;  // pool is owned by the device
};
}  // namespace ri
