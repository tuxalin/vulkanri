
#include <ri/Surface.h>

#include <cassert>
#include <util/math.h>
#include <ri/ApplicationInstance.h>
#include <ri/CommandPool.h>
#include <ri/DeviceContext.h>
#include <ri/RenderPass.h>
#include <ri/RenderTarget.h>
#include <ri/Texture.h>

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

    inline VkPhysicalDevice getDevicePhysicalHandle(const ri::DeviceContext& device)
    {
        return device.m_physicalDevice;
    }
    VkQueue getDeviceQueue(const ri::DeviceContext& device, int deviceOperation)
    {
        return device.m_queues[DeviceOperations::from(deviceOperation).get()];
    }
    uint32_t getDeviceQueueIndex(const ri::DeviceContext& device, int deviceOperation)
    {
        return device.m_queueIndices[DeviceOperations::from(deviceOperation).get()];
    }
}

Surface::Surface(const ApplicationInstance& instance, const Sizei& size, void* window, PresentMode mode)
    : m_instance(instance)
    , m_size(size)
    , m_presentMode(mode)
{
    assert(window);

#if RI_PLATFORM == RI_PLATFORM_GLFW
    RI_CHECK_RESULT() = glfwCreateWindowSurface(detail::getVkHandle(m_instance), reinterpret_cast<GLFWwindow*>(window),
                                                nullptr, &m_surface);
#else
#error "Unknown platform!"
#endif
}

Surface::~Surface()
{
    if (!m_logicalDevice)
        return;

    delete m_renderPass;
    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
    vkDestroySurfaceKHR(detail::getVkHandle(m_instance), m_surface, nullptr);
    for (auto target : m_swapchainTargets)
        delete target;

    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore, nullptr);
}

void Surface::initialize(ri::DeviceContext& device)
{
    assert(!m_logicalDevice);
    assert(m_presentQueueIndex >= 0);

    m_logicalDevice = detail::getVkHandle(device);
    // set presentation queue
    vkGetDeviceQueue(m_logicalDevice, m_presentQueueIndex, 0, &m_presentQueue);
    assert(m_presentQueue);

    const SwapChainSupport   support       = determineSupport(device);
    const VkSurfaceFormatKHR surfaceFormat = detail::chooseSurfaceFormat(support.formats);
    m_extent                               = detail::chooseSurfaceExtent(support.capabilities, m_size);
    m_format                               = ColorFormat::from((int)surfaceFormat.format);

    // TODO: review this
    const uint32_t graphicsQueueIndex = detail::getDeviceQueueIndex(device, DeviceOperations::eGraphics);
    createSwapchain(support, surfaceFormat, graphicsQueueIndex);
    createRenderTargets(device);
    createCommandBuffers(device);

    // create semaphores
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    RI_CHECK_RESULT() = vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
    RI_CHECK_RESULT() = vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
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

    RI_CHECK_RESULT() = vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapchain);

    assert(m_swapchainTargets.size() == m_swapchainCommandBuffers.size());
}

inline void Surface::createRenderTargets(const ri::DeviceContext& device)
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, nullptr);
    std::vector<VkImage> swapchainImages(imageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, swapchainImages.data());

    m_swapchainTargets.resize(imageCount);

    std::vector<RenderTarget::AttachmentParams> attachments(1);

    auto& attachment = attachments[0];
    {
        attachment.format = m_format;
        // create a general compatible render pass
        RenderPass::AttachmentParams params;
        params.format = m_format;
        m_renderPass  = new RenderPass(device, params);
    }

    uint32_t i = 0;
    for (auto& target : m_swapchainTargets)
    {
        // TODO: find a nicer way for reference textures
        attachment.texture = detail::createReferenceTexture(swapchainImages[i++], TextureType::e2D, m_size);

        target = new RenderTarget(device, *m_renderPass, attachments);
        delete attachment.texture;
    }
}

inline void Surface::createCommandBuffers(ri::DeviceContext& device)
{
    m_swapchainCommandBuffers.resize(m_swapchainTargets.size(), nullptr);
    assert(!m_swapchainCommandBuffers.empty());
    device.commandPool().create(m_swapchainCommandBuffers);
}

uint32_t Surface::acquire(uint64_t timeout /*= std::numeric_limits<uint64_t>::max()*/)
{
    // acquire next avaiable target
    vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, timeout, m_imageAvailableSemaphore, VK_NULL_HANDLE,
                          &m_currentTargetIndex);
    return m_currentTargetIndex;
}

bool Surface::present(const ri::DeviceContext& device)
{
    assert(m_currentTargetIndex != 0xFFFF);

    // submit current target's command buffer
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = &m_imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        const auto handle                 = detail::getVkHandle(*m_swapchainCommandBuffers[m_currentTargetIndex]);
        submitInfo.pCommandBuffers        = &handle;

        const auto queueHandle = detail::getDeviceQueue(device, DeviceOperations::eGraphics);
        RI_CHECK_RESULT()      = vkQueueSubmit(queueHandle, 1, &submitInfo, VK_NULL_HANDLE);
    }

    // TODO: investigate why semaphore needed
    // submit presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pWaitSemaphores    = &m_renderFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_currentTargetIndex;
    presentInfo.pResults           = nullptr;

    auto res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    return !res;
}

void Surface::waitIdle()
{
    vkQueueWaitIdle(m_presentQueue);
}

void Surface::setPresentationQueue(const ri::DeviceContext& device)
{
    assert(m_presentQueueIndex == -1);
    const auto deviceHandle = detail::getDevicePhysicalHandle(device);

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
    const auto deviceHandle = detail::getDevicePhysicalHandle(device);

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
