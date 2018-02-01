#pragma once

#include <memory>
#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class CommandBuffer;

class CommandPool : util::noncopyable, public RenderObject<VkCommandPool>
{
public:
    CommandBuffer create(bool isPrimary = true);
    void          create(std::vector<CommandBuffer>& buffers, bool isPrimary = true);
    /// Creates a one time command buffer, must be always followed by an flush command.
    /// @note Calls begin on the created buffer.
    CommandBuffer begin();
    /// @note Also destroys the buffer and waits for completion.
    void end(CommandBuffer& buffer);

    void free(std::vector<CommandBuffer>& buffers);

private:
    // @param resetMode Allows any command buffer to be individually reset, via CommandBuffer::reset or implicit reset
    // called on begin.
    CommandPool(bool resetMode, DeviceCommandHint commandHint);
    ~CommandPool();

    DeviceCommandHint deviceCommandHint() const;
    bool              resetMode() const;

    void initialize(const DeviceContext& device, int queueIndex);

private:
    const DeviceContext*                      m_device = nullptr;
    DeviceCommandHint                         m_commandHint;
    std::vector<detail::CommandBufferStorage> m_oneTimeBuffers;
    bool                                      m_resetMode;

    friend class DeviceContext;  // pool is owned by the device
};

inline DeviceCommandHint CommandPool::deviceCommandHint() const
{
    return m_commandHint;
}

inline bool CommandPool::resetMode() const
{
    return m_resetMode;
}
}  // namespace ri
