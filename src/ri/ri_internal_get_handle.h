#pragma once

#include <ri/ApplicationInstance.h>
#include <ri/DeviceContext.h>
#include <ri/RenderPass.h>

namespace ri
{
namespace detail
{
    struct ApplicationInstance
    {
        VkInstance m_instance;
    };

    struct DeviceContext
    {
        using OperationIndices = std::array<int, (size_t)DeviceOperations::Count>;
        using OperationQueues  = std::array<VkQueue, (size_t)DeviceOperations::Count>;

        const ApplicationInstance&    m_instance;
        std::vector<DeviceOperations> m_requiredOperations;
        VkPhysicalDevice              m_physicalDevice;
        VkDevice                      m_device;
        OperationQueues               m_queues;
        OperationIndices              m_queueIndices;
    };

    struct RenderPass
    {
        VkRenderPass m_renderPass;
        VkDevice     m_logicalDevice;
    };

    inline VkRenderPass getVkHandle(const ri::RenderPass& obj)
    {
        static_assert(sizeof(ri::RenderPass) == sizeof(ri::detail::RenderPass), "INVALID_SIZES");
        return reinterpret_cast<const detail::RenderPass&>(obj).m_renderPass;
    }

    inline VkInstance getVkHandle(const ri::ApplicationInstance& obj)
    {
        static_assert(sizeof(ri::ApplicationInstance) == sizeof(ri::detail::ApplicationInstance), "INVALID_SIZES");
        return reinterpret_cast<const detail::ApplicationInstance&>(obj).m_instance;
    }

    inline VkPhysicalDevice getVkHandle(const ri::DeviceContext& obj)
    {
        static_assert(sizeof(ri::DeviceContext) == sizeof(ri::detail::DeviceContext), "INVALID_SIZES");
        return reinterpret_cast<const detail::DeviceContext&>(obj).m_physicalDevice;
    }

    inline VkDevice getVkLogicalHandle(const ri::DeviceContext& obj)
    {
        static_assert(sizeof(ri::DeviceContext) == sizeof(ri::detail::DeviceContext), "INVALID_SIZES");
        return reinterpret_cast<const detail::DeviceContext&>(obj).m_device;
    }
}
}
