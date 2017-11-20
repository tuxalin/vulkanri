#pragma once

#include <cassert>
#include <vector>
#include <vulkan/vulkan.h>
#include <ri/Size.h>

namespace ri
{
#define RI_TOKENPASTE(x, y) x##y
#define RI_TOKENPASTE2(x, y) RI_TOKENPASTE(x, y)
#define RI_CHECK_RESULT() const detail::CheckRes RI_TOKENPASTE2(res, __LINE__)

class Texture;
class Surface;
class ShaderPipeline;
class DeviceContext;

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

    const Texture* createReferenceTexture(VkImage handle, int type, const Sizei& size);

    VkPhysicalDevice getDevicePhysicalHandle(const ri::DeviceContext& device);
    VkQueue          getDeviceQueue(const ri::DeviceContext& device, int deviceOperation);
    uint32_t         getDeviceQueueIndex(const ri::DeviceContext& device, int deviceOperation);

}  // namespace detail
}  // namespace ri
