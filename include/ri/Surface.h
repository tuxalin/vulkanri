#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class ApplicationInstance;
class DeviceContext;
class CommandBuffer;

class Surface : util::noncopyable
{
public:
    // @note If the present mode is not available it'll fallback to the PresentMode::eNormal mode.
    Surface(const ApplicationInstance& instance, const Sizei& size, void* window,
            PresentMode mode = PresentMode::eNormal);
    ~Surface();

    Sizei       size() const;
    ColorFormat format() const;

    // Acquires the next image, must be called before any drawing operations.
    // @param timeout in nanoseconds for a image to become avaialable, by default disabled.
    void acquire(uint64_t timeout = std::numeric_limits<uint64_t>::max());
    // @warning Must always be called in pair with acquire.
    // @return true if the presentation was succesful.
    bool present(const ri::DeviceContext& device);
    // Wait for the presentation to finish synchronously
    void waitIdle();

private:
    struct SwapChainSupport
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    void             initialize(ri::DeviceContext& device);
    void             setPresentationQueue(const ri::DeviceContext& device);
    SwapChainSupport determineSupport(const ri::DeviceContext& device);

    void createSwapchain(const SwapChainSupport& support, const VkSurfaceFormatKHR& surfaceFormat,
                         uint32_t graphicsQueueIndex);
    void createImageViews();
    void createCommandBuffers(ri::DeviceContext& device);

private:
    const ApplicationInstance&  m_instance;
    VkSurfaceKHR                m_surface       = VK_NULL_HANDLE;
    VkSwapchainKHR              m_swapchain     = VK_NULL_HANDLE;
    VkDevice                    m_logicalDevice = VK_NULL_HANDLE;
    std::vector<VkImageView>    m_swapchainImageViews;
    std::vector<CommandBuffer*> m_imageCommandBuffers;
    int                         m_presentQueueIndex = -1;
    VkQueue                     m_presentQueue;
    Sizei                       m_size;
    PresentMode                 m_presentMode;
    ColorFormat                 m_format;
    VkExtent2D                  m_extent;
    uint32_t                    m_currentImageIndex = 0xFFFF;

    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;

    friend VkDeviceQueueCreateInfo ri::detail::attachSurfaceTo(Surface& surface, const DeviceContext& device);
    friend void                    ri::detail::initializeSurface(DeviceContext& device, Surface& surface);
};

inline Sizei Surface::size() const
{
    return Sizei(m_extent.width, m_extent.height);
}

inline ColorFormat Surface::format() const
{
    return m_format;
}
}  // namespace ri
