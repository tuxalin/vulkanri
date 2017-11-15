#pragma once

#include "Types.h"

namespace ri
{
class DeviceContext;

class ShaderModule
{
public:
    ShaderModule(const DeviceContext& device, const std::string& filename, ShaderStage stage);
    ~ShaderModule();

private:
    VkShaderModule m_shaderModule  = VK_NULL_HANDLE;
    VkDevice       m_logicalDevice = VK_NULL_HANDLE;
    ShaderStage    m_stage;
};
}
