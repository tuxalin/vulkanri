
#include <ri/Surface.h>

#include <cassert>
#include <limits>
#include <util/math.h>
#include <ri/ApplicationInstance.h>
#include <ri/CommandBuffer.h>
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

    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates,
                                 VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        assert(false);
        return VK_FORMAT_UNDEFINED;
    }

    VkFormat chooseDepthFormat(VkPhysicalDevice physicalDevice, VkFormat preferredFormat)
    {
        std::vector<VkFormat> formats = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                         VK_FORMAT_D24_UNORM_S8_UINT};

        auto found = std::find(formats.begin(), formats.end(), preferredFormat);
        assert(found != formats.end());
        if (found != formats.begin())
            std ::swap(*found, *formats.begin());

        return findSupportedFormat(physicalDevice, formats, VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
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

Surface::Surface(const ApplicationInstance& instance, const Sizei& size, const SurfaceCreateParams& params,
                 PresentMode mode)
    : m_instance(instance)
    , m_size(size)
    , m_presentMode(mode)
    , m_depthFormat((ColorFormat)params.depthBufferType)
{
#if RI_PLATFORM == RI_PLATFORM_GLFW
    assert(params.window);
    RI_CHECK_RESULT() = glfwCreateWindowSurface(detail::getVkHandle(m_instance), params.window, nullptr, &m_handle);
#elif RI_PLATFORM == RI_PLATFORM_WINDOWS
    assert(param.hwnd && param.hinstance);
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance                   = params.hinstance;
    info.hwnd                        = params.hwnd;
    info.flags                       = params.flags;
    vkCreateWin32SurfaceKHR(detail::getVkHandle(m_instance), &info, nullptr, &m_surface);
#else
#error "Unknown platform!"
#endif
}

Surface::~Surface()
{
    if (!m_device)
        return;

    cleanup(true);

    vkDestroySurfaceKHR(detail::getVkHandle(m_instance), m_handle, nullptr);
}

void Surface::cleanup(bool cleanSwapchain)
{
    delete m_renderPass, m_renderPass = nullptr;

    if (cleanSwapchain)
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr), m_swapchain = VK_NULL_HANDLE;

    for (auto& target : m_swapchainTargets)
        delete target, target = nullptr;

    delete m_depthTexture, m_depthTexture = nullptr;
    vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
}

void Surface::recreate(ri::DeviceContext& device, const Sizei& size)
{
    m_size = size;

    device.waitIdle();
    device.commandPool().free(reinterpret_cast<std::vector<CommandBuffer>&>(m_swapchainCommandBuffers));

    auto oldSwapchain = m_swapchain;
    cleanup(false);
    create(device);

    vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
}

void Surface::initialize(ri::DeviceContext& device)
{
    assert(!m_device);
    assert(m_presentQueueIndex >= 0);

    m_device = detail::getVkHandle(device);
    // set presentation queue
    vkGetDeviceQueue(m_device, m_presentQueueIndex, 0, &m_presentQueue);
    assert(m_presentQueue);

    assert(m_swapchain == VK_NULL_HANDLE);
    create(device);
}

void Surface::create(ri::DeviceContext& device)
{
    const SwapChainSupport   support       = determineSupport(device);
    const VkSurfaceFormatKHR surfaceFormat = detail::chooseSurfaceFormat(support.formats);
    m_extent                               = detail::chooseSurfaceExtent(support.capabilities, m_size);
    m_format                               = ColorFormat::from((int)surfaceFormat.format);

    // TODO: review this
    const uint32_t graphicsQueueIndex = detail::getDeviceQueueIndex(device, DeviceOperation::eGraphics);
    createSwapchain(support, surfaceFormat, graphicsQueueIndex);
    createCommandBuffers(device);
    createDepthBuffer(device);
    createRenderTargets(device);

    // create semaphores
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    RI_CHECK_RESULT_MSG("couldn't create surface's available semaphore") =
        vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
    RI_CHECK_RESULT_MSG("couldn't create surface's finished semaphore") =
        vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
}

void Surface::createDepthBuffer(const ri::DeviceContext& device)
{
    if (m_depthFormat == ColorFormat::eUndefined)
        return;

    // create depth buffer
    m_depthFormat =
        (ColorFormat)detail::chooseDepthFormat(detail::getDevicePhysicalHandle(device), (VkFormat)m_depthFormat);

    TextureParams params;
    params.format  = m_depthFormat;
    params.size    = size();
    params.flags   = TextureUsageFlags::eDepthStencil;
    m_depthTexture = new Texture(device, params);
    m_depthTexture->setTagName(tagName() + "_depthTexture");

    m_oneTimeCommandBuffer.cast().begin(RecordFlags::eOneTime);
    m_depthTexture->transitionImageLayout((TextureLayoutType)VK_IMAGE_LAYOUT_UNDEFINED,
                                          (TextureLayoutType)VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                          m_oneTimeCommandBuffer.cast());
    m_oneTimeCommandBuffer.cast().end();
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
    createInfo.surface                  = m_handle;
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
    createInfo.oldSwapchain             = m_swapchain;  // reuse current swapchain for new one

    const uint32_t queueFamilyIndices[] = {graphicsQueueIndex, (uint32_t)m_presentQueueIndex};
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

    RI_CHECK_RESULT_MSG("couldn't crate surface's swapchain") =
        vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);

    assert(m_swapchainTargets.size() == m_swapchainCommandBuffers.size());
}

inline void Surface::createRenderTargets(const ri::DeviceContext& device)
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    std::vector<VkImage> swapchainImages(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, swapchainImages.data());

    m_swapchainTargets.resize(imageCount);

    const size_t                   attachmentCount = m_depthTexture ? 2 : 1;
    RenderTarget::AttachmentParams attachments[2];
    {
        // create a compatible render pass
        RenderPass::AttachmentParams params[2];
        params[0].format      = m_format;
        params[0].finalLayout = TextureLayoutType::ePresentSrc;

        params[1].format      = m_depthFormat;
        params[1].finalLayout = TextureLayoutType::eDepthStencilOptimal;
        params[1].storeColor  = false;

        m_renderPass = new RenderPass(device, params, attachmentCount);

        for (size_t i = 0; i < attachmentCount; ++i)
        {
            m_attachments.emplace_back();
            auto& attachment       = m_attachments.back();
            attachment.format      = params[i].format;
            attachment.samples     = params[i].samples;
            attachment.finalLayout = params[i].finalLayout;
        }
    }
    {  // setup the depth attachment
        attachments[1].texture       = m_depthTexture;
        attachments[1].takeOwnership = false;
    }

    auto&    colorAttachment = attachments[0];
    uint32_t i               = 0;
    for (auto& target : m_swapchainTargets)
    {
        // TODO: find a nicer way for reference textures
        colorAttachment.texture =
            detail::createReferenceTexture(swapchainImages[i++], TextureType::e2D, m_format.get(), m_size);

        target = new RenderTarget(device, *m_renderPass, attachments, attachmentCount);
        delete colorAttachment.texture;
    }
}

inline void Surface::createCommandBuffers(ri::DeviceContext& device)
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);

    m_swapchainCommandBuffers.resize(imageCount);
    assert(!m_swapchainCommandBuffers.empty());

    device.commandPool().create(reinterpret_cast<std::vector<CommandBuffer>&>(m_swapchainCommandBuffers));
    m_oneTimeCommandBuffer.cast() =
        device.commandPool(DeviceOperation::eTransfer, DeviceCommandHint::eRecorded).create(true);

#ifndef NDEBUG
    int i = 0;
    for (auto& buffer : m_swapchainCommandBuffers)
    {
        buffer.setTagName(tagName() + ":CommandBuffer" + std::to_string(i++));
    }
#endif  // !NDEBUG
}

uint32_t Surface::acquire(uint64_t timeout /*= std::numeric_limits<uint64_t>::max()*/)
{
    // acquire next available target
    auto res = vkAcquireNextImageKHR(m_device, m_swapchain, timeout, m_imageAvailableSemaphore, VK_NULL_HANDLE,
                                     &m_currentTargetIndex);
    assert(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);
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
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = &m_renderFinishedSemaphore;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        const auto handle                 = detail::getVkHandle(m_swapchainCommandBuffers[m_currentTargetIndex].cast());
        submitInfo.pCommandBuffers        = &handle;

        const auto queueHandle = detail::getDeviceQueue(device, DeviceOperation::eGraphics);
        RI_CHECK_RESULT_MSG("error at queue submit for surface present") =
            vkQueueSubmit(queueHandle, 1, &submitInfo, VK_NULL_HANDLE);
    }

    // submit presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_renderFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_currentTargetIndex;
    presentInfo.pResults           = nullptr;

    auto res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    assert(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);
    return res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR;
}

void Surface::waitIdle()
{
    vkQueueWaitIdle(m_presentQueue);
}

void Surface::setPresentationQueue(const ri::DeviceContext& device)
{
    assert(m_presentQueueIndex == -1);
    const auto physicalDeviceHandle = detail::getDevicePhysicalHandle(device);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &queueFamilyCount, queueFamilies.data());

    // search a presentation queue family
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeviceHandle, i, m_handle, &presentSupport);

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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceHandle, m_handle, &support.capabilities);

    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, m_handle, &count, nullptr);
    if (count != 0)
    {
        support.formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, m_handle, &count, support.formats.data());
    }

    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, m_handle, &count, nullptr);
    if (count != 0)
    {
        support.presentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, m_handle, &count, support.presentModes.data());
    }

    return support;
}
}  // namespace ri
