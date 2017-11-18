#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;

class ShaderModule : util::noncopyable
{
public:
    ShaderModule(const DeviceContext& device, const std::string& filename, ShaderStage stage);
    ~ShaderModule();

    ShaderStage stage() const;

private:
    VkShaderModule m_shaderModule  = VK_NULL_HANDLE;
    VkDevice       m_logicalDevice = VK_NULL_HANDLE;
    ShaderStage    m_stage;
};

inline ShaderStage ShaderModule::stage() const
{
    return m_stage;
}
}
