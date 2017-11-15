
#include <ri/ShaderModule.h>

#include <fstream>

#include "ri_internal_get_handle.h"

namespace ri
{
ShaderModule::ShaderModule(const DeviceContext& device, const std::string& filename, ShaderStage stage)
    : m_logicalDevice(detail::getVkLogicalHandle(device))
    , m_stage(stage)
{
    std::ifstream file(filename + ".spv", std::ios::ate | std::ios::binary);
    assert(file.is_open());

    std::vector<char> shaderCode((size_t)file.tellg());
    file.seekg(0);
    file.read(shaderCode.data(), shaderCode.size());
    file.close();

    VkShaderModuleCreateInfo createInfo = {};

    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data());

    auto res = vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &m_shaderModule);
    assert(!res);
}

ShaderModule::~ShaderModule()
{
    assert(m_logicalDevice);
    vkDestroyShaderModule(m_logicalDevice, m_shaderModule, nullptr);
}
}
