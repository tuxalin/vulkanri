
#include <ri/ValidationReport.h>

#include <cassert>
#include <iostream>
#include <util/common.h>
#include <util/safe_enum.h>
#include <ri/ApplicationInstance.h>

namespace ri
{
namespace
{
    const std::vector<std::string> kValidationLayers = {"VK_LAYER_LUNARG_standard_validation"};

    SAFE_ENUM_DECLARE(DebugReportFlags,                                            //
                      eInfo        = VK_DEBUG_REPORT_INFORMATION_BIT_EXT,          //
                      eWarning     = VK_DEBUG_REPORT_WARNING_BIT_EXT,              //
                      ePerformance = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,  //
                      eError       = VK_DEBUG_REPORT_ERROR_BIT_EXT,                //
                      eDebug       = VK_DEBUG_REPORT_DEBUG_BIT_EXT);
}
ValidationReport::ValidationReport(const ApplicationInstance&   instance,
                                   ReportLevel                  level /*= ReportFlags::eError*/,
                                   const VkAllocationCallbacks* allocator /*= nullptr*/)
    : m_instance(instance)
    , m_allocator(allocator)
{
    assert_if(kEnabled, checkValidationLayerSupport());

    if (!kEnabled)
        return;

    VkDebugReportCallbackCreateInfoEXT createInfo = {};

    createInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags       = (VkDebugReportFlagsEXT)level;
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
    std::cerr << "Validation layer: " << DebugReportFlags::from(flags).str() << ": " << msg << std::endl;
    assert(DebugReportFlags::from(flags) != DebugReportFlags::eError);
    return VK_FALSE;
}
}  // namespace ri
