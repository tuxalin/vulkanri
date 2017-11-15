#pragma once

#include <vector>

#include "ApplicationInstance.h"

namespace ri
{
class ValidationReport
{
public:
#ifdef NDEBUG
    static const bool kEnabled = false;
#else
    static const bool kEnabled = true;
#endif

    ValidationReport(const ApplicationInstance& instance, const VkAllocationCallbacks* allocator = nullptr);
    ~ValidationReport();

    static std::vector<const char*> getActiveLayers();

private:
    bool checkValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT      flags,
                                                        VkDebugReportObjectTypeEXT objType,
                                                        uint64_t                   obj,
                                                        size_t                     location,
                                                        int32_t                    code,
                                                        const char*                layerPrefix,
                                                        const char*                msg,
                                                        void*                      userData);

private:
    const ApplicationInstance&   m_instance;
    VkDebugReportCallbackEXT     m_callback;
    const VkAllocationCallbacks* m_allocator;
};
}
