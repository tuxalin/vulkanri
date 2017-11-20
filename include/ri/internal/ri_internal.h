#pragma once

#include <cassert>
#include <vector>
#include <vulkan/vulkan.h>
#include <ri/Size.h>

namespace ri
{
class Texture;
class Surface;
class DeviceContext;
class ShaderPipeline;
class CommandPool;

#define RI_TOKENPASTE(x, y) x##y
#define RI_TOKENPASTE2(x, y) RI_TOKENPASTE(x, y)
#define RI_CHECK_RESULT() const detail::CheckRes RI_TOKENPASTE2(res, __LINE__)

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

    template <class DetailRenderClass, class RenderClass>
    auto getVkHandleImpl(const RenderClass& obj);
}
}  // namespace ri
