#pragma once

#include <array>
#include <vector>
#include <util/noncopyable.h>
#include <ri/CommandPool.h>
#include <ri/Types.h>

namespace ri
{
class ApplicationInstance;
class Surface;

class DeviceContext : util::noncopyable
{
public:
    DeviceContext(const ApplicationInstance& instance, DeviceCommandHint commandHint = DeviceCommandHint::eRecord);
    ~DeviceContext();

    void initialize(Surface& surface, const std::vector<DeviceFeatures>& requiredFeatures,
                    const std::vector<DeviceOperations>& requiredOperations);
    //@note Will attach the surfaces to the context.
    void initialize(const std::vector<Surface*>& surfaces, const std::vector<DeviceFeatures>& requiredFeatures,
                    const std::vector<DeviceOperations>& requiredOperations);

    CommandPool&       commandPool();
    const CommandPool& commandPool() const;

    void waitIdle();

    const std::vector<DeviceOperations>& requiredOperations() const;

private:
    using FamilyQueueIndex = int;
    using OperationIndices = std::array<FamilyQueueIndex, (size_t)DeviceOperations::Count>;
    using OperationQueues  = std::array<VkQueue, (size_t)DeviceOperations::Count>;

    uint32_t         deviceScore(VkPhysicalDevice device, const std::vector<DeviceFeatures>& requiredFeatures);
    OperationIndices searchQueueFamilies(const std::vector<DeviceOperations>& requiredOperations);
    void             createDevice(const std::vector<Surface*>& surfaces, const VkPhysicalDeviceFeatures& deviceFeatures,
                                  const std::vector<const char*>& deviceExtensions);
    std::vector<VkDeviceQueueCreateInfo> determineQueueCreation(const std::vector<Surface*>& surfaces);

private:
    const ApplicationInstance&    m_instance;
    std::vector<DeviceOperations> m_requiredOperations;
    VkPhysicalDevice              m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                      m_handle         = VK_NULL_HANDLE;
    OperationQueues               m_queues;
    OperationIndices              m_queueIndices;
    CommandPool                   m_commandPool;
};

inline void DeviceContext::initialize(Surface& surface, const std::vector<DeviceFeatures>& requiredFeatures,
                                      const std::vector<DeviceOperations>& requiredOperations)
{
    const std::vector<Surface*> data(1, &surface);
    initialize(data, requiredFeatures, requiredOperations);
}

inline const std::vector<DeviceOperations>& DeviceContext::requiredOperations() const
{
    assert(!m_requiredOperations.empty());
    return m_requiredOperations;
}

inline CommandPool& DeviceContext::commandPool()
{
    return m_commandPool;
}
inline const CommandPool& DeviceContext::commandPool() const
{
    return m_commandPool;
}

}  // namespace ri
