#pragma once

#include <cassert>
#include <vector>
#include <vulkan/vulkan.h>

namespace ri
{
class Surface;
class DeviceContext;
class ShaderPipeline;
class CommandPool;

#define RI_CHECK_RESULT() const detail::CheckRes res

namespace detail
{
    struct CheckRes
    {
        CheckRes(VkResult res)
        {
            assert(!res);
        }
    };

    VkDeviceQueueCreateInfo attachSurfaceTo(Surface& surface, const DeviceContext& device);
    void                    initializeSurface(DeviceContext& device, Surface& surface);

    const std::vector<VkPipelineShaderStageCreateInfo>& getStageCreateInfos(const ShaderPipeline& pipeline);
}
}
