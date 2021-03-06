
#include <ri/ApplicationInstance.h>

#include <cassert>
#include <iostream>
#include <ri/ValidationReport.h>

namespace ri
{
namespace
{
    void checkRequiredExtensionsPresent(std::vector<VkExtensionProperties> availableExt, const char** requiredExt,
                                        int requiredExtCount)
    {
        for (int i = 0; i < requiredExtCount; i++)
        {
            bool found = false;
            for (const auto& extension : availableExt)
            {
                if (strcmp(requiredExt[i], extension.extensionName))
                {
                    found = true;
                }
            }
            if (!found)
            {
                // missing extension
                assert(false);
            }
        }
#ifndef NDEBUG
        std::cout << "extension requirement fulfilled" << std::endl;
#endif  // ! NDEBUG
    }
}

ApplicationInstance::ApplicationInstance(const std::string& name, const std::string& engineName)
{
    assert(!name.empty());

    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = name.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = engineName.empty() ? name.c_str() : engineName.c_str();
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo     = &appInfo;

    std::vector<const char*> extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount    = extensions.size();
    createInfo.ppEnabledExtensionNames  = extensions.data();
    std::vector<const char*> layers     = ri::ValidationReport::getActiveLayers();
    createInfo.enabledLayerCount        = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames      = layers.data();

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());
#ifndef NDEBUG
    std::cout << "available extensions:" << std::endl;
    for (const auto& extensionProperty : extensionProperties)
    {
        std::cout << "\t" << extensionProperty.extensionName << std::endl;
    }
#endif  // ! NDEBUG

    RI_CHECK_RESULT_MSG("couldn't create application instance") = vkCreateInstance(&createInfo, nullptr, &m_handle);
}

ApplicationInstance::~ApplicationInstance()
{
    vkDestroyInstance(m_handle, nullptr);
}

std::vector<const char*> ApplicationInstance::getRequiredExtensions()
{
    std::vector<const char*> extensions;

#if RI_PLATFORM == RI_PLATFORM_GLFW
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int i = 0; i < glfwExtensionCount; i++)
    {
        extensions.push_back(glfwExtensions[i]);
    }

#ifndef NDEBUG
    std::cout << "glfw required extensions:" << std::endl;
    for (unsigned int i = 0; i < glfwExtensionCount; i++)
    {
        std::cout << "\t" << glfwExtensions[i] << std::endl;
    }
#endif  // ! NDEBUG
#endif

    if (ValidationReport::kEnabled)
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}
}  // namespace ri
