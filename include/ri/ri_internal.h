#pragma once

#include <vulkan/vulkan.h>

namespace ri
{
class Surface;
class DeviceContext;

namespace detail
{
    VkDeviceQueueCreateInfo attachSurfaceTo(Surface& surface, const DeviceContext& device);
    void                    initializeSurface(const DeviceContext& device, Surface& surface);
}
}
