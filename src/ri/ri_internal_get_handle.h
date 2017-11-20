#pragma once

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
    struct ApplicationInstance
    {
        VkInstance m_handle;
    };

    struct CommandBuffer
    {
        VkCommandBuffer m_handle;
    };

    struct CommandPool
    {
        VkDevice          m_device;
        VkCommandPool     m_handle;
        DeviceCommandHint m_commandHint;
    };

    struct DeviceContext
    {
        using OperationIndices = std::array<int, (size_t)DeviceOperations::Count>;
        using OperationQueues  = std::array<VkQueue, (size_t)DeviceOperations::Count>;

        const ApplicationInstance&    m_instance;
        std::vector<DeviceOperations> m_requiredOperations;
        VkPhysicalDevice              m_physicalDevice;
        VkDevice                      m_handle;
        OperationQueues               m_queues;
        OperationIndices              m_queueIndices;
        CommandPool*                  m_commandPool;
    };

    struct RenderPass
    {
        VkRenderPass            m_handle;
        VkDevice                m_logicalDevice;
        std::vector<ClearValue> m_clearValues;
        Sizei                   m_renderArea;
        int32_t                 m_renderAreaOffset[2];
    };

    struct Texture
    {
        VkImage     m_handle;
        TextureType m_type;
        Sizei       m_size;
    };

    struct RenderTarget
    {
        VkFramebuffer            m_handle;
        VkDevice                 m_logicalDevice;
        std::vector<VkImageView> m_attachments;
    };

    template <class DetailRenderClass, class RenderClass>
    auto getVkHandleImpl(const RenderClass& obj)
    {
        static_assert(sizeof(RenderClass) == sizeof(DetailRenderClass), "INVALID_SIZES");
        static_assert(offsetof(RenderClass, m_handle) == offsetof(DetailRenderClass, m_handle), "INVALID_CLASS_LAYOUT");
        return reinterpret_cast<const DetailRenderClass&>(obj).m_handle;
    }

    inline VkFramebuffer getVkHandle(const ri::RenderTarget& obj)
    {
        return getVkHandleImpl<ri::detail::RenderTarget>(obj);
    }

    inline VkImage getVkHandle(const ri::Texture& obj)
    {
        return getVkHandleImpl<ri::detail::Texture>(obj);
    }

    inline VkRenderPass getVkHandle(const ri::RenderPass& obj)
    {
        return getVkHandleImpl<ri::detail::RenderPass>(obj);
    }

    inline VkCommandBuffer getVkHandle(const ri::CommandBuffer& obj)
    {
        return getVkHandleImpl<ri::detail::CommandBuffer>(obj);
    }

    inline VkInstance getVkHandle(const ri::ApplicationInstance& obj)
    {
        return getVkHandleImpl<ri::detail::ApplicationInstance>(obj);
    }

    inline VkDevice getVkHandle(const ri::DeviceContext& obj)
    {
        return getVkHandleImpl<ri::detail::DeviceContext>(obj);
    }
}  // namespace detail
}  // namespace ri
