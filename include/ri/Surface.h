#pragma once

#include "ApplicationInstance.h"
#include "Types.h"

namespace ri
{
class DeviceContext;

class Surface
{
public:
    // @note If the present mode is not available it'll fallback to the PresentMode::eNormal mode.
    Surface(const ApplicationInstance& instance, const Size& size, void* window,
            PresentMode mode = PresentMode::eNormal);
    ~Surface();

private:
    struct SwapChainSupport
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    void             initialize(const ri::DeviceContext& device);
    void             setPresentationQueue(const ri::DeviceContext& device);
    SwapChainSupport determineSupport(const ri::DeviceContext& device);

private:
    const ApplicationInstance& m_instance;
    VkSurfaceKHR               m_surface       = VK_NULL_HANDLE;
    VkSwapchainKHR             m_swapchain     = VK_NULL_HANDLE;
    VkDevice                   m_logicalDevice = VK_NULL_HANDLE;
    std::vector<VkImageView>   m_swapChainImageViews;
    int                        m_presentQueueIndex = -1;
    VkQueue                    m_presentQueue;
    Size                       m_size;
    PresentMode                m_presentMode;
    VkSurfaceFormatKHR         m_format;
    VkExtent2D                 m_extent;

    friend VkDeviceQueueCreateInfo ri::detail::attachSurfaceTo(Surface& surface, const DeviceContext& device);
    friend void                    ri::detail::initializeSurface(const DeviceContext& device, Surface& surface);
};
}
