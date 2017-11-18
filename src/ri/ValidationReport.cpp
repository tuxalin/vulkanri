
#include <ri/ValidationReport.h>

#include "ri_internal_get_handle.h"
#include <cassert>
#include <iostream>
#include <util/common.h>
#include <ri/ApplicationInstance.h>

const std::vector<std::string> kValidationLayers = {"VK_LAYER_LUNARG_standard_validation"};

namespace ri
{
ValidationReport::ValidationReport(const ApplicationInstance&   instance,
                                   const VkAllocationCallbacks* allocator /*= nullptr*/)
    : m_instance(instance)
    , m_allocator(allocator)
{
    assert_if(kEnabled, checkValidationLayerSupport());

    if (!kEnabled)
        return;

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(detail::getVkHandle(m_instance),
                                                                          "vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
    {
        func(detail::getVkHandle(m_instance), &createInfo, nullptr, &m_callback);
    }
    else
    {
        assert(false);
    }
}

ValidationReport::~ValidationReport()
{
    if (!kEnabled)
        return;

    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(detail::getVkHandle(m_instance),
                                                                           "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr && m_callback)
    {
        func(detail::getVkHandle(m_instance), m_callback, nullptr);
    }
}

std::vector<const char*> ValidationReport::getActiveLayers()
{
    std::vector<const char*> res;
    if (kEnabled)
    {
        for (const auto& layerName : kValidationLayers)
            res.push_back(layerName.c_str());
    }

    return res;
}

bool ValidationReport::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const std::string& layerName : kValidationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (layerName == std::string(layerProperties.layerName))
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL ValidationReport::debugCallback(VkDebugReportFlagsEXT      flags,
                                                               VkDebugReportObjectTypeEXT objType,
                                                               uint64_t                   obj,
                                                               size_t                     location,
                                                               int32_t                    code,
                                                               const char*                layerPrefix,
                                                               const char*                msg,
                                                               void*                      userData)
{
    std::cerr << "validation layer: " << msg << std::endl;
    return VK_FALSE;
}
}
