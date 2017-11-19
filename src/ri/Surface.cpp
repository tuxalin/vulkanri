
#include <ri/Surface.h>

#include "ri_internal_get_handle.h"
#include <cassert>
#include <util/math.h>
#include <ri/ApplicationInstance.h>

namespace ri
{
namespace detail
{
    VkDeviceQueueCreateInfo attachSurfaceTo(ri::Surface& surface, const ri::DeviceContext& device)
    {
        surface.setPresentationQueue(device);

        // create present queue
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex        = surface.m_presentQueueIndex;
        queueCreateInfo.queueCount              = 1;
        // TODO: add support to predefine priority per queue
        float queuePriority              = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        return queueCreateInfo;
    }

    void initializeSurface(ri::DeviceContext& device, ri::Surface& surface)
    {
        surface.initialize(device);
    }

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // surface has no preferred format, so use sRGB and BGRA storage by default
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
            return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

        auto found = std::find_if(availableFormats.begin(), availableFormats.end(), [](const auto& format) {
            return format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        });
        if (found != availableFormats.end())
            return *found;

        return availableFormats[0];
    }

    VkExtent2D chooseSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const ri::Sizei& size)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        const VkExtent2D actualExtent = {
            math::clamp(size.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            math::clamp(size.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
        return actualExtent;
    }

    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                       VkPresentModeKHR                     preferredMode)
    {
        auto found = std::find(availablePresentModes.begin(), availablePresentModes.end(), preferredMode);
        if (found != availablePresentModes.end())
            return preferredMode;

        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return VK_PRESENT_MODE_MAILBOX_KHR;
            else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                bestMode = availablePresentMode;
        }

        return bestMode;
    }
}

Surface::Surface(const ApplicationInstance& instance, const Sizei& size, void* window, PresentMode mode)
    : m_instance(instance)
    , m_size(size)
    , m_presentMode(mode)
{
    assert(window);

#if RI_PLATFORM == RI_PLATFORM_GLFW
    auto res = glfwCreateWindowSurface(detail::getVkHandle(m_instance), reinterpret_cast<GLFWwindow*>(window), nullptr,
                                       &m_surface);
    assert(!res);
#else
#error "Unknown platform!"
#endif
}

Surface::~Surface()
{
    if (!m_logicalDevice)
        return;

    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
    vkDestroySurfaceKHR(detail::getVkHandle(m_instance), m_surface, nullptr);
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
        vkDestroyImageView(m_logicalDevice, m_swapchainImageViews[i], nullptr);

    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore, nullptr);
}

void Surface::initialize(ri::DeviceContext& device)
{
    assert(!m_logicalDevice);
    assert(m_presentQueueIndex >= 0);

    m_logicalDevice = detail::getVkLogicalHandle(device);
    // set presentation queue
    vkGetDeviceQueue(m_logicalDevice, m_presentQueueIndex, 0, &m_presentQueue);
    assert(m_presentQueue);

    const SwapChainSupport   support       = determineSupport(device);
    const VkSurfaceFormatKHR surfaceFormat = detail::chooseSurfaceFormat(support.formats);
    m_extent                               = detail::chooseSurfaceExtent(support.capabilities, m_size);
    m_format                               = ColorFormat::from((int)surfaceFormat.format);

    // TODO: review this
    const auto&    deviceDetail       = reinterpret_cast<const detail::DeviceContext&>(device);
    const uint32_t graphicsQueueIndex = deviceDetail.m_queueIndices[DeviceOperations::eGraphics];
    createSwapchain(support, surfaceFormat, graphicsQueueIndex);
    createImageViews();
    createCommandBuffers(device);

    // create semaphores
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    auto res = vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
    assert(!res);
    res = vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
    assert(!res);
}

void Surface::createSwapchain(const SwapChainSupport& support, const VkSurfaceFormatKHR& surfaceFormat,
                              uint32_t graphicsQueueIndex)
{
    // get supported image count
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount)
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);
    else
        // unlimited, so use at least 3
        imageCount = std::max(imageCount, 3u);

    const VkPresentModeKHR presentMode =
        detail::choosePresentMode(support.presentModes, static_cast<VkPresentModeKHR>(m_presentMode.get()));

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                  = m_surface;
    createInfo.minImageCount            = imageCount;
    createInfo.imageFormat              = surfaceFormat.format;
    createInfo.imageColorSpace          = surfaceFormat.colorSpace;
    createInfo.imageExtent              = m_extent;
    createInfo.imageArrayLayers         = 1;
    createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform             = support.capabilities.currentTransform;
    createInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode              = presentMode;
    createInfo.clipped                  = VK_TRUE;
    // TODO: support swapchain recreation, eg. on resize
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    uint32_t queueFamilyIndices[] = {graphicsQueueIndex, (uint32_t)m_presentQueueIndex};
    if (queueFamilyIndices[0] != queueFamilyIndices[1] && queueFamilyIndices[0] >= 0)
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
    }

    auto res = vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapchain);
    assert(!res);
}

inline void Surface::createImageViews()
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, nullptr);
    std::vector<VkImage> swapchainImages(imageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, swapchainImages.data());

    m_swapchainImageViews.resize(imageCount);
    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                 = swapchainImages[i];
        createInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                = (VkFormat)m_format;
        createInfo.components.r          = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g          = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b          = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a          = VK_COMPONENT_SWIZZLE_IDENTITY;
        // no mipmapping or layers
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        auto res = vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapchainImageViews[i]);
        assert(!res);
    }
}

inline void Surface::createCommandBuffers(ri::DeviceContext& device)
{
    m_imageCommandBuffers.resize(m_swapchainImageViews.size(), nullptr);
    assert(!m_imageCommandBuffers.empty());
    device.commandPool().create(m_imageCommandBuffers);
}

void Surface::acquire(uint64_t timeout /*= std::numeric_limits<uint64_t>::max()*/)
{
    vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, timeout, m_imageAvailableSemaphore, VK_NULL_HANDLE,
                          &m_currentImageIndex);
}

bool Surface::present(const ri::DeviceContext& device)
{
    assert(m_currentImageIndex != 0xFFFF);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore          waitSemaphores[] = {m_imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount         = 1;
    submitInfo.pWaitSemaphores            = waitSemaphores;
    submitInfo.pWaitDstStageMask          = waitStages;
    submitInfo.commandBufferCount         = 1;
    const auto handle                     = detail::getVkHandle(*m_imageCommandBuffers[m_currentImageIndex]);
    submitInfo.pCommandBuffers            = &handle;

    const auto& deviceDetail = reinterpret_cast<const detail::DeviceContext&>(device);
    auto        res = vkQueueSubmit(deviceDetail.m_queues[DeviceOperations::eGraphics], 1, &submitInfo, VK_NULL_HANDLE);
    assert(!res);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &m_currentImageIndex;
    presentInfo.pResults        = nullptr;

    res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    return !res;
}

void Surface::waitIdle()
{
    vkQueueWaitIdle(m_presentQueue);
}

void Surface::setPresentationQueue(const ri::DeviceContext& device)
{
    assert(m_presentQueueIndex == -1);
    auto deviceHandle = detail::getVkHandle(device);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &queueFamilyCount, queueFamilies.data());

    // search a presentation queue family
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(deviceHandle, i, m_surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport)
        {
            m_presentQueueIndex = i;
            break;
        }
        ++i;
    }
}

Surface::SwapChainSupport Surface::determineSupport(const ri::DeviceContext& device)
{
    auto deviceHandle = detail::getVkHandle(device);

    SwapChainSupport support;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceHandle, m_surface, &support.capabilities);

    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, m_surface, &count, nullptr);
    if (count != 0)
    {
        support.formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, m_surface, &count, support.formats.data());
    }

    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, m_surface, &count, nullptr);
    if (count != 0)
    {
        support.presentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, m_surface, &count, support.presentModes.data());
    }

    return support;
}
}  // namespace ri
