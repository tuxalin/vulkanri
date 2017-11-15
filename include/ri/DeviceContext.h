#pragma once

#include <array>
#include <vector>

#include "ApplicationInstance.h"
#include "Types.h"

namespace ri
{
class Surface;

class DeviceContext
{
public:
    DeviceContext(const ApplicationInstance& instance);
    ~DeviceContext();

    void create(Surface& surface, const std::vector<DeviceFeatures>& requiredFeatures,
                const std::vector<DeviceOperations>& requiredOperations);
    //@note Will attach the surfaces to the context.
    void create(const std::vector<Surface*>& surfaces, const std::vector<DeviceFeatures>& requiredFeatures,
                const std::vector<DeviceOperations>& requiredOperations);

    const std::vector<DeviceOperations>& requiredOperations() const;

private:
    using FamilyQueueIndex = int;
    using OperationIndices = std::array<FamilyQueueIndex, (size_t)DeviceOperations::Count>;
    using OperationQueues  = std::array<VkQueue, (size_t)DeviceOperations::Count>;

    uint32_t         deviceScore(VkPhysicalDevice device, const std::vector<DeviceFeatures>& requiredFeatures);
    OperationIndices searchQueueFamilies(const std::vector<DeviceOperations>& requiredOperations);
    void             createDevice(const std::vector<Surface*>& surfaces, const VkPhysicalDeviceFeatures& deviceFeatures,
                                  const std::vector<const char*>& deviceExtensions);

private:
    const ApplicationInstance&    m_instance;
    std::vector<DeviceOperations> m_requiredOperations;
    VkPhysicalDevice              m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                      m_device         = VK_NULL_HANDLE;
    OperationQueues               m_queues;
    OperationIndices              m_queueIndices;
};

inline void DeviceContext::create(Surface& surface, const std::vector<DeviceFeatures>& requiredFeatures,
                                  const std::vector<DeviceOperations>& requiredOperations)
{
    const std::vector<Surface*> data(1, &surface);
    create(data, requiredFeatures, requiredOperations);
}

inline const std::vector<DeviceOperations>& DeviceContext::requiredOperations() const
{
    assert(!m_requiredOperations.empty());
    return m_requiredOperations;
}
}
