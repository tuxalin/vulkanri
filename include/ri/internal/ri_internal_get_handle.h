#pragma once

#include <vulkan/vulkan.h>

#include "RenderObject.h"

namespace ri
{
class Texture;
class Surface;
class DeviceContext;
class ShaderPipeline;
class CommandPool;
class CommandBuffer;
class ApplicationInstance;
class RenderTarget;
class RenderPass;

namespace detail
{
    template <typename HandleClass>
    HandleClass getVkHandle(const RenderObject<HandleClass>& obj)
    {
        return obj.m_handle;
    }
    inline VkDevice getVkHandle(const DeviceContext& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkDevice>&>(obj));
    }
    inline VkInstance getVkHandle(const ApplicationInstance& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkInstance>&>(obj));
    }
    inline VkCommandPool getVkHandle(const CommandPool& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkCommandPool>&>(obj));
    }
    inline VkCommandBuffer getVkHandle(const CommandBuffer& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkCommandBuffer>&>(obj));
    }
    inline VkImage getVkHandle(const Texture& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkImage>&>(obj));
    }
    inline VkFramebuffer getVkHandle(const RenderTarget& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkFramebuffer>&>(obj));
    }
    inline VkRenderPass getVkHandle(const RenderPass& obj)
    {
        return getVkHandle(reinterpret_cast<const RenderObject<VkRenderPass>&>(obj));
    }
}  // namespace detail
}  // namespace ri
