
#include <ri/DeviceContext.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <util/common.h>
#include <util/iterator.h>
#include <ri/CommandPool.h>
#include <ri/ValidationReport.h>

namespace ri
{
namespace
{
    const std::unordered_map<DeviceFeature, const char*> kDeviceStringMap = {
        {DeviceFeature::eSwapchain, VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    int getFlagFrom(DeviceOperation type)
    {
        switch (type.get())
        {
            case DeviceOperation::eGraphics:
                return VK_QUEUE_GRAPHICS_BIT;
            case DeviceOperation::eTransfer:
                return VK_QUEUE_TRANSFER_BIT;
            case DeviceOperation::eCompute:
                return VK_QUEUE_COMPUTE_BIT;
            default:
                assert(false);
        }
        return 0;
    }

    std::pair<VkPhysicalDeviceFeatures, std::vector<const char*> > getDevicesFeatures(
        const std::vector<DeviceFeature>& requiredFeatures)
    {
        std::pair<VkPhysicalDeviceFeatures, std::vector<const char*> > result;

        auto& deviceFeatures = result.first;
        auto& extensionNames = result.second;
        for (auto feature : requiredFeatures)
        {
            switch (feature.get())
            {
                case DeviceFeature::eGeometryShader:
                    deviceFeatures.geometryShader = VK_TRUE;
                    break;
                case DeviceFeature::eTesselationShader:
                    deviceFeatures.tessellationShader = VK_TRUE;
                    break;
                case DeviceFeature::eFloat64:
                    deviceFeatures.shaderFloat64 = VK_TRUE;
                    break;
                case DeviceFeature::eWireframe:
                    deviceFeatures.fillModeNonSolid = VK_TRUE;
                    break;
                case DeviceFeature::eAnisotropy:
                    deviceFeatures.samplerAnisotropy = VK_TRUE;
                    break;
                case DeviceFeature::eSampleRateShading:
                    deviceFeatures.sampleRateShading = VK_TRUE;
                    break;
                default:
                    auto found = kDeviceStringMap.find(feature);
                    assert(found != kDeviceStringMap.end());
                    extensionNames.push_back(found->second);
                    break;
            }
        }
        return result;
    }
}  // namespace

DeviceContext::DeviceContext(const ApplicationInstance& instance)
    : m_instance(instance)
{
    m_commandPools.fill(nullptr);
}

DeviceContext::~DeviceContext()
{
    for (auto commandPool : m_commandPools)
        delete commandPool;
    vkDestroyDevice(m_handle, nullptr);
}

void DeviceContext::initialize(const std::vector<Surface*>&        surfaces,
                               const std::vector<DeviceFeature>&   requiredFeatures,
                               const std::vector<DeviceOperation>& requiredOperations,
                               const CommandPoolParam&             commandParam /*= CommandPoolParam()*/)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(detail::getVkHandle(m_instance), &deviceCount, nullptr);
    assert(deviceCount);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(detail::getVkHandle(m_instance), &deviceCount, devices.data());

    // determine device based on score
    std::vector<uint32_t> deviceScores(deviceCount);
    uint32_t              index = 0;
    std::for_each(deviceScores.begin(), deviceScores.end(), [this, devices, requiredFeatures, &index](uint32_t& score) {
        score = deviceScore(devices[index++], requiredFeatures);
    });

    auto maxScore = std::max_element(deviceScores.begin(), deviceScores.end());
    if (*maxScore > 0)
    {
        size_t index     = util::index_of(deviceScores, maxScore);
        index            = std::distance(deviceScores.begin(), maxScore);
        m_physicalDevice = devices[index];
    }
    assert(m_physicalDevice != VK_NULL_HANDLE);
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

    // create a logical device
    {
        m_requiredOperations                                        = requiredOperations;
        const auto&                                features         = getDevicesFeatures(requiredFeatures);
        const std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = attachSurfaces(surfaces.data(), surfaces.size());
        createDevice(queueCreateInfos, features.first, features.second);
        assert(m_handle != VK_NULL_HANDLE);
    }

    addCommandPool(DeviceOperation::eGraphics, commandParam);
    m_defaultCommandPool = &commandPool(DeviceOperation::eGraphics, commandParam.hints);

    for (Surface* surface : surfaces)
    {
        ri::detail::initializeSurface(*this, *surface);
    }
}

void DeviceContext::addCommandPool(DeviceOperation operation, const CommandPoolParam& param)
{
    static_assert(DeviceCommandHint::Count == 2, "");
    static_assert(DeviceOperation::eGraphics == 0, "");

    // 0 for graphics pool, 1 for compute
    auto& commandPool = m_commandPools[(operation.get() == DeviceOperation::eCompute) * 2 + param.hints.get()];
    if (!commandPool)
    {
        commandPool = new CommandPool(param.resetMode, param.hints, operation);
        commandPool->initialize(*this, m_queueIndices[static_cast<size_t>(operation)]);
    }
    assert(param.resetMode == commandPool->resetMode());
}

uint32_t DeviceContext::deviceScore(VkPhysicalDevice device, const std::vector<DeviceFeature>& requiredFeatures)
{
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &m_deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t             score        = 0;
    VkPhysicalDeviceType validTypes[] = {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU};
    uint32_t             scoreTypes[] = {1000, 10};

    auto found = std::find_if(std::begin(validTypes), std::end(validTypes),
                              [this](auto type) { return m_deviceProperties.deviceType == type; });
    if (found == std::end(validTypes))
        return 0;

    score = scoreTypes[util::index_of(validTypes, found)];
    score += m_deviceProperties.limits.maxImageDimension2D;

    std::vector<VkExtensionProperties> availableExtensions;
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        availableExtensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    }

    bool hasAllFeatures = true;
    for (auto feature : requiredFeatures)
    {
        switch (feature.get())
        {
            case DeviceFeature::eGeometryShader:
                hasAllFeatures &= deviceFeatures.geometryShader == VK_TRUE;
                break;
            case DeviceFeature::eTesselationShader:
                hasAllFeatures &= deviceFeatures.tessellationShader == VK_TRUE;
                break;
            case DeviceFeature::eFloat64:
                hasAllFeatures &= deviceFeatures.shaderFloat64 == VK_TRUE;
                break;
            case DeviceFeature::eAnisotropy:
                hasAllFeatures &= deviceFeatures.samplerAnisotropy == VK_TRUE;
                break;
            case DeviceFeature::eWireframe:
                hasAllFeatures &= deviceFeatures.fillModeNonSolid == VK_TRUE;
                break;
            case DeviceFeature::eSampleRateShading:
                hasAllFeatures &= deviceFeatures.sampleRateShading == VK_TRUE;
                break;
            default:
                auto found = kDeviceStringMap.find(feature);
                assert(found != kDeviceStringMap.end());
                const auto& extensionName = found->second;
                hasAllFeatures &= std::find_if(availableExtensions.begin(), availableExtensions.end(),
                                               [extensionName](const VkExtensionProperties& e) {
                                                   return std::string(e.extensionName) == extensionName;
                                               }) != availableExtensions.end();
                break;
        }
        if (!hasAllFeatures)
        {
            score = 0;
            break;
        }
    }

    return score;
}

DeviceContext::OperationIndices DeviceContext::searchQueueFamilies(
    const std::vector<DeviceOperation>& requiredOperations)
{
    assert(requiredOperations.size() < (size_t)DeviceOperation::Count);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    OperationIndices indices;
    indices.fill(-1);
    for (DeviceOperation type : requiredOperations)
    {
        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueCount == 0)
                continue;

            const VkQueueFlags flag = getFlagFrom(type);
            if (queueFamily.queueFlags & flag)
            {
                indices[(size_t)type] = i;
                break;
            }

            i++;
        }
    }
    return indices;
}

inline std::vector<VkDeviceQueueCreateInfo> DeviceContext::attachSurfaces(const SurfacePtr* surfaces,
                                                                          size_t            surfacesCount)
{
    m_queueIndices = searchQueueFamilies(m_requiredOperations);

    // determine queues to create
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<FamilyQueueIndex> queueFamilies;
    for (size_t i = 0; i < m_queueIndices.size(); ++i)
    {
        FamilyQueueIndex index = m_queueIndices[i];
        if (index < 0 || queueFamilies.find(index) != queueFamilies.end())  // index already used
            continue;

        queueFamilies.insert(index);

        queueCreateInfos.emplace_back();
        auto& queueCreateInfo            = queueCreateInfos.back();
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount       = 1;

        // TODO: add support to predefine priority per queue
        float queuePriority              = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;
    }

    // get the presentation queue for each surface
    for (size_t i = 0; i < surfacesCount; ++i)
    {
        Surface& surface         = *surfaces[i];
        auto     queueCreateInfo = ri::detail::attachSurfaceTo(surface, *this);
        if (queueFamilies.find(queueCreateInfo.queueFamilyIndex) == queueFamilies.end())  // index already used
        {
            assert(queueCreateInfo.queueFamilyIndex >= 0);
            queueCreateInfos.push_back(queueCreateInfo);
        }
    }
    return queueCreateInfos;
}

void DeviceContext::createDevice(const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos,
                                 const VkPhysicalDeviceFeatures&             deviceFeatures,
                                 const std::vector<const char*>&             deviceExtensions)
{
    // create logical device
    {
        VkDeviceCreateInfo createInfo   = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos    = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = queueCreateInfos.size();
        // device specific extensions
        createInfo.pEnabledFeatures        = &deviceFeatures;
        createInfo.enabledExtensionCount   = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        std::vector<const char*> layers = ValidationReport::getActiveLayers();
        if (ValidationReport::kEnabled)
        {
            createInfo.enabledLayerCount   = layers.size();
            createInfo.ppEnabledLayerNames = layers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        RI_CHECK_RESULT_MSG("couldn't create logical device") =
            vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_handle);

        m_queues.fill(VK_NULL_HANDLE);
        for (size_t i = 0; i < m_queueIndices.size(); ++i)
        {
            FamilyQueueIndex index = m_queueIndices[i];
            if (index < 0)
                continue;

            vkGetDeviceQueue(m_handle, index, 0, &m_queues[i]);
        }
    }
}

}  // namespace ri
