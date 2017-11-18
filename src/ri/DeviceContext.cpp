
#include <ri/DeviceContext.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_set>

#include <util/common.h>
#include <util/iterator.h>

#include <ri/ValidationReport.h>

#include "ri_internal_get_handle.h"

namespace ri
{
namespace
{
    const std::unordered_map<DeviceFeatures, const char*> kDeviceStringMap = {
        {DeviceFeatures::eSwapchain, VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    int getFlagFrom(DeviceOperations type)
    {
        switch (type.get())
        {
            case DeviceOperations::eGraphics:
                return VK_QUEUE_GRAPHICS_BIT;
            case DeviceOperations::eTransfer:
                return VK_QUEUE_TRANSFER_BIT;
            case DeviceOperations::eCompute:
                return VK_QUEUE_COMPUTE_BIT;
            default:
                assert(false);
        }
        return 0;
    }

    std::pair<VkPhysicalDeviceFeatures, std::vector<const char*> > getDevicesFeatures(
        const std::vector<DeviceFeatures>& requiredFeatures)
    {
        std::pair<VkPhysicalDeviceFeatures, std::vector<const char*> > result;

        auto& deviceFeatures = result.first;
        auto& extensionNames = result.second;
        for (auto feature : requiredFeatures)
        {
            switch (feature.get())
            {
                case DeviceFeatures::eGeometryShader:
                    deviceFeatures.geometryShader = VK_TRUE;
                    break;
                case DeviceFeatures::eFloat64:
                    deviceFeatures.shaderFloat64 = VK_TRUE;
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
    static_assert(
        offsetof(ri::DeviceContext, m_physicalDevice) == offsetof(ri::detail::DeviceContext, m_physicalDevice),
        "INVALID_CLASS_LAYOUT");
    static_assert(offsetof(ri::DeviceContext, m_device) == offsetof(ri::detail::DeviceContext, m_device),
                  "INVALID_CLASS_LAYOUT");
}

DeviceContext::~DeviceContext()
{
    vkDestroyDevice(m_device, nullptr);
}

void DeviceContext::create(const std::vector<Surface*>&         surfaces,
                           const std::vector<DeviceFeatures>&   requiredFeatures,
                           const std::vector<DeviceOperations>& requiredOperations)
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

    // create a logical device
    m_requiredOperations = requiredOperations;
    const auto& features = getDevicesFeatures(requiredFeatures);
    createDevice(surfaces, features.first, features.second);
    assert(m_device != VK_NULL_HANDLE);
}

uint32_t DeviceContext::deviceScore(VkPhysicalDevice device, const std::vector<DeviceFeatures>& requiredFeatures)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures   deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t             score        = 0;
    VkPhysicalDeviceType validTypes[] = {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU};
    uint32_t             scoreTypes[] = {1000, 10};

    auto found = std::find_if(std::begin(validTypes), std::end(validTypes),
                              [deviceProperties](auto type) { return deviceProperties.deviceType == type; });
    if (found == std::end(validTypes))
        return 0;

    score = scoreTypes[util::index_of(validTypes, found)];
    score += deviceProperties.limits.maxImageDimension2D;

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
            case DeviceFeatures::eGeometryShader:
                hasAllFeatures &= deviceFeatures.geometryShader == VK_TRUE;
                break;
            case DeviceFeatures::eFloat64:
                hasAllFeatures &= deviceFeatures.shaderFloat64 == VK_TRUE;
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
    const std::vector<DeviceOperations>& requiredOperations)
{
    assert(requiredOperations.size() < (size_t)DeviceOperations::Count);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    OperationIndices indices;
    indices.fill(-1);
    for (DeviceOperations type : requiredOperations)
    {
        std::cout << type.str();
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

void DeviceContext::createDevice(const std::vector<Surface*>&    surfaces,
                                 const VkPhysicalDeviceFeatures& deviceFeatures,
                                 const std::vector<const char*>& deviceExtensions)
{
    m_queueIndices = searchQueueFamilies(m_requiredOperations);

    // queues to create
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
    for (Surface* surface : surfaces)
    {
        auto queueCreateInfo = ri::detail::attachSurfaceTo(*surface, *this);
        if (queueFamilies.find(queueCreateInfo.queueFamilyIndex) == queueFamilies.end())  // index already used
        {
            assert(queueCreateInfo.queueFamilyIndex >= 0);
            queueCreateInfos.push_back(queueCreateInfo);
        }
    }

    // create logical device
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

    VkResult res = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    assert(!res);

    m_queues.fill(VK_NULL_HANDLE);
    for (size_t i = 0; i < m_queueIndices.size(); ++i)
    {
        FamilyQueueIndex index = m_queueIndices[i];
        if (index < 0)
            continue;

        vkGetDeviceQueue(m_device, index, 0, &m_queues[i]);
    }

    for (Surface* surface : surfaces)
    {
        ri::detail::initializeSurface(*this, *surface);
    }
}
}  // namespace ri
