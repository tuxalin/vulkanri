
#include <ri/ShaderModule.h>

#include "ri_internal_get_handle.h"
#include <fstream>

namespace ri
{
ShaderModule::ShaderModule(const DeviceContext& device, const std::string& filename, ShaderStage stage)
    : m_logicalDevice(detail::getVkHandle(device))
    , m_stage(stage)
{
    std::ifstream file(filename + ".spv", std::ios::ate | std::ios::binary);
    assert(file.is_open());

#ifdef NDEBUG
    std::vector<char> code((size_t)file.tellg());
#else
    m_code.resize((size_t)file.tellg());
    auto& code = m_code;
#endif

    file.seekg(0);
    file.read(code.data(), code.size());
    file.close();

    VkShaderModuleCreateInfo createInfo = {};

    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    auto res = vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &m_handle);
    assert(!res);
}

ShaderModule::~ShaderModule()
{
    assert(m_logicalDevice);
    vkDestroyShaderModule(m_logicalDevice, m_handle, nullptr);
}

#ifndef NDEBUG
// O(m*n), no need for speed
const char* sstrstr(const char* haystack, const char* needle, size_t length)
{
    size_t needle_length = strlen(needle);
    size_t i;

    for (i = 0; i < length; i++)
    {
        if (i + needle_length > length)
            return NULL;

        if (strncmp(&haystack[i], needle, needle_length) == 0)
            return &haystack[i];
    }
    return NULL;
}

bool ShaderModule::hasProcedure(const std::string& name) const
{
    assert(!m_code.empty());
    std::string t     = m_code.data();
    auto        found = sstrstr(m_code.data(), name.c_str(), m_code.size());
    // TODO: also check if spir-v procedure
    return found != nullptr;
}
#endif
}
