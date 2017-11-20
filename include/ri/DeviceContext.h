#pragma once

#include <array>
#include <vector>
#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class ApplicationInstance;
class Surface;
class CommandPool;

class DeviceContext : util::noncopyable, public detail::RenderObject<VkDevice>
{
public:
    DeviceContext(const ApplicationInstance& instance, bool commandResetMode,
                  DeviceCommandHint commandHint = DeviceCommandHint::eRecorded);
    ~DeviceContext();

    void initialize(Surface& surface, const std::vector<DeviceFeature>& requiredFeatures,
                    const std::vector<DeviceOperation>& requiredOperations);
    //@note Will attach the surfaces to the context.
    void initialize(const std::vector<Surface*>& surfaces, const std::vector<DeviceFeature>& requiredFeatures,
                    const std::vector<DeviceOperation>& requiredOperations);

    CommandPool&       commandPool();
    const CommandPool& commandPool() const;

    void waitIdle();

    const std::vector<DeviceOperation>& requiredOperations() const;

private:
    using FamilyQueueIndex = int;
    using OperationIndices = std::array<FamilyQueueIndex, (size_t)DeviceOperation::Count>;
    using OperationQueues  = std::array<VkQueue, (size_t)DeviceOperation::Count>;

    uint32_t         deviceScore(VkPhysicalDevice device, const std::vector<DeviceFeature>& requiredFeatures);
    OperationIndices searchQueueFamilies(const std::vector<DeviceOperation>& requiredOperations);
    void             createDevice(const std::vector<Surface*>& surfaces, const VkPhysicalDeviceFeatures& deviceFeatures,
                                  const std::vector<const char*>& deviceExtensions);
    std::vector<VkDeviceQueueCreateInfo> determineQueueCreation(const std::vector<Surface*>& surfaces);

private:
    const ApplicationInstance&   m_instance;
    std::vector<DeviceOperation> m_requiredOperations;
    VkPhysicalDevice             m_physicalDevice = VK_NULL_HANDLE;
    OperationQueues              m_queues;
    OperationIndices             m_queueIndices;
    CommandPool*                 m_commandPool;

    friend VkPhysicalDevice detail::getDevicePhysicalHandle(const ri::DeviceContext& device);
    friend VkQueue          detail::getDeviceQueue(const ri::DeviceContext& device, int deviceOperation);
    friend uint32_t         detail::getDeviceQueueIndex(const ri::DeviceContext& device, int deviceOperation);
};

inline void DeviceContext::initialize(Surface& surface, const std::vector<DeviceFeature>& requiredFeatures,
                                      const std::vector<DeviceOperation>& requiredOperations)
{
    const std::vector<Surface*> data(1, &surface);
    initialize(data, requiredFeatures, requiredOperations);
}

inline const std::vector<DeviceOperation>& DeviceContext::requiredOperations() const
{
    assert(!m_requiredOperations.empty());
    return m_requiredOperations;
}

inline CommandPool& DeviceContext::commandPool()
{
    return *m_commandPool;
}
inline const CommandPool& DeviceContext::commandPool() const
{
    return *m_commandPool;
}

}  // namespace ri
