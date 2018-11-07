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

class DeviceContext : util::noncopyable, public RenderObject<VkDevice>
{
public:
    struct CommandPoolParam
    {
        DeviceCommandHint hints     = DeviceCommandHint::eRecorded;
        bool              resetMode = false;
    };

    DeviceContext(const ApplicationInstance& instance);
    ~DeviceContext();

    void initialize(Surface&                            surface,             //
                    const std::vector<DeviceFeature>&   requiredFeatures,    //
                    const std::vector<DeviceOperation>& requiredOperations,  //
                    const CommandPoolParam&             commandParam = CommandPoolParam());
    /// @note Will attach the surfaces to the context.
    void initialize(const std::vector<Surface*>&        surfaces,            //
                    const std::vector<DeviceFeature>&   requiredFeatures,    //
                    const std::vector<DeviceOperation>& requiredOperations,  //
                    const CommandPoolParam&             commandParam = CommandPoolParam());

    /// Will return the default command pool.
    CommandPool&       commandPool();
    CommandPool&       commandPool(DeviceOperation operation, DeviceCommandHint commandHint);
    const CommandPool& commandPool(DeviceOperation operation, DeviceCommandHint commandHint) const;
    /// @note By default the graphics command buffer with the give param is created.
    CommandPool& addCommandPool(DeviceOperation operation, const CommandPoolParam& param);

    void waitIdle();

    const DeviceProperties& deviceProperties() const;

    TextureProperties textureProperties(ColorFormat format, TextureType type, TextureTiling tiling,
                                        uint32_t flags) const;

    const std::vector<DeviceOperation>& requiredOperations() const;

private:
    using SurfacePtr       = Surface*;
    using FamilyQueueIndex = int;
    using OperationIndices = std::array<FamilyQueueIndex, (size_t)DeviceOperation::Count>;
    using OperationQueues  = std::array<VkQueue, (size_t)DeviceOperation::Count>;

    uint32_t         deviceScore(VkPhysicalDevice device, const std::vector<DeviceFeature>& requiredFeatures);
    OperationIndices searchQueueFamilies(const std::vector<DeviceOperation>& requiredOperations);
    void             createDevice(const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos,
                                  const VkPhysicalDeviceFeatures& deviceFeatures, const std::vector<const char*>& deviceExtensions);
    std::vector<VkDeviceQueueCreateInfo> attachSurfaces(const SurfacePtr* surfaces, size_t surfacesCount);

    static size_t commandPoolIndex(DeviceOperation operation, DeviceCommandHint commandHint)
    {
        static_assert(DeviceCommandHint::Count == 2, "");
        static_assert(DeviceCommandHint::eRecorded == 0, "");
        static_assert(DeviceCommandHint::eTransient == 1, "");
        return size_t(operation) * DeviceCommandHint::Count + commandHint.get();
    }

private:
    static const size_t cPoolSize = DeviceOperation::Count + DeviceCommandHint::Count;

    const ApplicationInstance&          m_instance;
    std::vector<DeviceOperation>        m_requiredOperations;
    VkPhysicalDevice                    m_physicalDevice = VK_NULL_HANDLE;
    OperationQueues                     m_queues;
    OperationIndices                    m_queueIndices;
    CommandPool*                        m_defaultCommandPool = nullptr;
    std::array<CommandPool*, cPoolSize> m_commandPools;
    VkPhysicalDeviceMemoryProperties    m_memoryProperties;
    DeviceProperties                    m_deviceProperties;

    friend VkPhysicalDevice detail::getDevicePhysicalHandle(const ri::DeviceContext& device);
    friend VkQueue          detail::getDeviceQueue(const ri::DeviceContext& device, int deviceOperation);
    friend uint32_t         detail::getDeviceQueueIndex(const ri::DeviceContext& device, int deviceOperation);
    friend const VkPhysicalDeviceMemoryProperties& detail::getDeviceMemoryProperties(const ri::DeviceContext& device);
};

inline void DeviceContext::initialize(Surface&                            surface,             //
                                      const std::vector<DeviceFeature>&   requiredFeatures,    //
                                      const std::vector<DeviceOperation>& requiredOperations,  //
                                      const CommandPoolParam&             commandParam /*= CommandPoolParam()*/)
{
    const std::vector<Surface*> data(1, &surface);
    initialize(data, requiredFeatures, requiredOperations, commandParam);
}

inline const DeviceProperties& DeviceContext::deviceProperties() const
{
    return m_deviceProperties;
}

inline TextureProperties DeviceContext::textureProperties(ColorFormat format, TextureType type, TextureTiling tiling,
                                                          uint32_t flags) const
{
    VkImageFormatProperties props;
    vkGetPhysicalDeviceImageFormatProperties(m_physicalDevice, (VkFormat)format, (VkImageType)type,
                                             (VkImageTiling)tiling, flags, 0, &props);
    return props;
}

inline const std::vector<DeviceOperation>& DeviceContext::requiredOperations() const
{
    assert(!m_requiredOperations.empty());
    return m_requiredOperations;
}

inline CommandPool& DeviceContext::commandPool()
{
    assert(m_defaultCommandPool);
    return *m_defaultCommandPool;
}

inline CommandPool& DeviceContext::commandPool(DeviceOperation operation, DeviceCommandHint commandHint)
{
    auto& commandPool = m_commandPools[commandPoolIndex(operation, commandHint)];
    assert(commandPool);
    return *commandPool;
}

inline const CommandPool& DeviceContext::commandPool(DeviceOperation operation, DeviceCommandHint commandHint) const
{
    const auto& commandPool = m_commandPools[commandPoolIndex(operation, commandHint)];
    assert(commandPool);
    return *commandPool;
}

inline void DeviceContext::waitIdle()
{
    vkDeviceWaitIdle(m_handle);
}

namespace detail
{
    inline VkPhysicalDevice getDevicePhysicalHandle(const ri::DeviceContext& device)
    {
        return device.m_physicalDevice;
    }
    inline VkQueue getDeviceQueue(const ri::DeviceContext& device, int deviceOperation)
    {
        return device.m_queues[DeviceOperation::from(deviceOperation).get()];
    }
    inline uint32_t getDeviceQueueIndex(const ri::DeviceContext& device, int deviceOperation)
    {
        return device.m_queueIndices[DeviceOperation::from(deviceOperation).get()];
    }
    inline const VkPhysicalDeviceMemoryProperties& detail::getDeviceMemoryProperties(const ri::DeviceContext& device)
    {
        return device.m_memoryProperties;
    }
}  // namespace detail

}  // namespace ri
