#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace ri
{
class Surface;
class DeviceContext;
class ShaderPipeline;

namespace detail
{
    VkDeviceQueueCreateInfo attachSurfaceTo(Surface& surface, const DeviceContext& device);
    void                    initializeSurface(const DeviceContext& device, Surface& surface);
    const std::vector<VkPipelineShaderStageCreateInfo>& getStageCreateInfos(const ShaderPipeline& pipeline);
}
}
