#pragma once

#include <util/noncopyable.h>
#include <ri/Types.h>

namespace ri
{
class DeviceContext;

class ShaderModule : util::noncopyable, public detail::RenderObject<VkShaderModule>
{
public:
    ShaderModule(const DeviceContext& device, const std::string& filename, ShaderStage stage);
    ~ShaderModule();

    ShaderStage stage() const;

#ifndef NDEBUG
    bool hasProcedure(const std::string& name) const;
#endif

private:
    VkDevice    m_logicalDevice = VK_NULL_HANDLE;
    ShaderStage m_stage;
#ifndef NDEBUG
    std::vector<char> m_code;
#endif  // !NDEBUG
};

inline ShaderStage ShaderModule::stage() const
{
    return m_stage;
}
}  // namespace ri
