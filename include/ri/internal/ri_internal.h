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
#define RI_CHECK_RESULT_MSG(msg) const detail::CheckRes RI_TOKENPASTE2(res, __LINE__)

class Texture;
class Surface;
class ShaderPipeline;
class CommandBuffer;
class DeviceContext;
class RenderPipeline;
class ComputePipeline;
class VertexDescription;
template <typename HandleClass>
class RenderObject;
class DescriptorPool;

namespace detail
{
    struct CheckRes
    {
        CheckRes(VkResult res)
        {
            assert(!res);
        }
    };
    struct TextureDescriptorInfo
    {
        VkImageView   imageView;
        VkSampler     sampler;
        VkImageLayout layout;
    };

    VkDeviceQueueCreateInfo attachSurfaceTo(Surface& surface, const DeviceContext& device);
    void                    initializeSurface(DeviceContext& device, Surface& surface);

    const std::vector<VkPipelineShaderStageCreateInfo>& getStageCreateInfos(const ShaderPipeline& pipeline);

    VkImageView           getImageViewHandle(const ri::Texture& texture);
    const Texture*        createReferenceTexture(VkImage handle, int type, int format, const Sizei& size);
    TextureDescriptorInfo getTextureDescriptorInfo(const Texture& texture);
    VkImageView           createExtraImageView(const Texture& texture, uint32_t baseMipLevel, uint32_t baseArrayLayer);
    VkImageAspectFlags    getImageAspectFlags(VkFormat format);
    VkImageType           getImageType(int type);

    VkPhysicalDevice                        getDevicePhysicalHandle(const ri::DeviceContext& device);
    VkQueue                                 getDeviceQueue(const ri::DeviceContext& device, int deviceOperation);
    uint32_t                                getDeviceQueueIndex(const ri::DeviceContext& device, int deviceOperation);
    const VkPhysicalDeviceMemoryProperties& getDeviceMemoryProperties(const ri::DeviceContext& device);

    const std::vector<VkVertexInputBindingDescription>&   getBindingDescriptions(const VertexDescription& layout);
    const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptons(const VertexDescription& layout);

    VkPipelineLayout getPipelineLayout(const RenderPipeline& pipeline);
    VkPipelineLayout getPipelineLayout(const ComputePipeline& pipeline);

    template <typename HandleClass>
    HandleClass getVkHandle(const RenderObject<HandleClass>& obj);

}  // namespace detail
}  // namespace ri
