#pragma once

#include <util/noncopyable.h>
#include <ri/Size.h>
#include <ri/Types.h>

namespace ri
{
class ApplicationInstance;
class DeviceContext;
class CommandBuffer;
class RenderTarget;
class RenderPass;
struct RenderPassAttachment;

class Surface : util::noncopyable, public RenderObject<VkSurfaceKHR>
{
public:
    struct Attachment
    {
        ColorFormat       format;
        TextureLayoutType finalLayout;
        uint32_t          samples;
    };

    // @note If the present mode is not available it'll fallback to the PresentMode::eNormal mode.
    Surface(const ApplicationInstance& instance, const Sizei& size, const SurfaceCreateParams& params,
            PresentMode mode = PresentMode::eNormal);
    ~Surface();

    Sizei                          size() const;
    ColorFormat                    format() const;
    ColorFormat                    depthFormat() const;
    uint32_t                       swapCount() const;
    uint32_t                       msaaSamples() const;
    const std::vector<Attachment>& attachments() const;

    CommandBuffer&      commandBuffer(uint32_t index);
    RenderPass&         renderPass();
    const RenderPass&   renderPass() const;
    const RenderTarget& renderTarget(uint32_t index) const;

    // Acquires the next image, must be called before any drawing operations.
    // @param timeout in nanoseconds for a image to become available, by default disabled.
    // @return active/available index of the swapchain.
    uint32_t acquire(uint64_t timeout = std::numeric_limits<uint64_t>::max());
    // @warning Must always be called in pair with acquire.
    // @return true if the presentation was successful.
    bool present(const ri::DeviceContext& device);
    // Wait for the presentation to finish synchronously
    void waitIdle();

    // @warning Must be called when the surface is invalidated, eg. when an resize happens.
    void recreate(ri::DeviceContext& device, const Sizei& size);

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

    void create(ri::DeviceContext& device);
    void createSwapchain(const SwapChainSupport& support, const VkSurfaceFormatKHR& surfaceFormat,
                         uint32_t graphicsQueueIndex);
    void createExtraBuffers(const ri::DeviceContext& device);
    void createRenderTargets(const ri::DeviceContext& device);
    void createCommandBuffers(ri::DeviceContext& device);

    void cleanup(bool cleanSwapchain);

private:
    const ApplicationInstance&                m_instance;
    VkSwapchainKHR                            m_swapchain = VK_NULL_HANDLE;
    VkDevice                                  m_device    = VK_NULL_HANDLE;
    std::vector<RenderTarget*>                m_swapchainTargets;
    std::vector<detail::CommandBufferStorage> m_swapchainCommandBuffers;
    detail::CommandBufferStorage              m_oneTimeCommandBuffer;
    int                                       m_presentQueueIndex = -1;
    VkQueue                                   m_presentQueue;
    Sizei                                     m_size;
    PresentMode                               m_presentMode;
    ColorFormat                               m_format;
    VkExtent2D                                m_extent;
    uint32_t                                  m_currentTargetIndex = 0xFFFF;
    RenderPass*                               m_renderPass         = nullptr;
    ColorFormat                               m_depthFormat;
    Texture*                                  m_depthTexture     = nullptr;
    Texture*                                  m_msaaColorTexture = nullptr;
    std::vector<Attachment>                   m_attachments;
    uint32_t                                  m_msaaSamples;

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

inline ColorFormat Surface::depthFormat() const
{
    return m_depthFormat;
}

inline uint32_t Surface::swapCount() const
{
    return m_swapchainCommandBuffers.size();
}

inline uint32_t Surface::msaaSamples() const
{
    return m_msaaSamples;
}

inline const std::vector<Surface::Attachment>& Surface::attachments() const
{
    return m_attachments;
}

inline CommandBuffer& Surface::commandBuffer(uint32_t index)
{
    auto& res = m_swapchainCommandBuffers[index].cast();
    return res;
}

inline RenderPass& Surface::renderPass()
{
    return *m_renderPass;
}

inline const RenderPass& Surface::renderPass() const
{
    return *m_renderPass;
}

inline const RenderTarget& Surface::renderTarget(uint32_t index) const
{
    const auto& res = m_swapchainTargets[index];
    return *res;
}
}  // namespace ri
