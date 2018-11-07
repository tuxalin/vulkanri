#pragma once

#include <array>
#include <util/iterator.h>
#include <util/noncopyable.h>
#include <ri/ShaderModule.h>

namespace ri
{
class ShaderPipeline : util::noncopyable, public TagableObject
{
public:
    ShaderPipeline();
    ~ShaderPipeline();

    ///@note Takes ownerwship of the shader module.
    void addStage(const ShaderModule* shader, const std::string& procedure = "main");
    void addStage(const ShaderModule& shader, const std::string& procedure = "main");
    void removeStage(ShaderStage stage);

private:
    using ShaderModules = std::array<const ShaderModule*, (size_t)ShaderStage::Count>;

    ShaderModules                                       m_shaders;
    std::vector<VkPipelineShaderStageCreateInfo>        m_stageInfos;
    std::array<std::string, (size_t)ShaderStage::Count> m_stageProcedures;

    friend const std::vector<VkPipelineShaderStageCreateInfo>& detail::getStageCreateInfos(
        const ShaderPipeline& pipeline);
};

namespace detail
{
    struct ShaderModule : public RenderObject<VkShaderModule>
    {
        VkDevice m_device;
        int      m_stageFlags;
#ifndef NDEBUG
        std::vector<char> m_code;
#endif  // !NDEBUG
    };

    inline const std::vector<VkPipelineShaderStageCreateInfo>& getStageCreateInfos(const ShaderPipeline& pipeline)
    {
        return pipeline.m_stageInfos;
    }
}

inline ShaderPipeline::ShaderPipeline()
{
    m_shaders.fill(nullptr);
}

inline ShaderPipeline::~ShaderPipeline()
{
    for (auto shader : m_shaders)
        delete shader;
}

inline void ShaderPipeline::addStage(const ShaderModule* shader, const std::string& procedure /*= "main"*/)
{
    auto& currentShader = m_shaders[shader->stage().ordinal()];
    if (currentShader)
        delete currentShader;

    currentShader = shader;
    addStage(*shader, procedure);
}

inline void ShaderPipeline::addStage(const ShaderModule& shader, const std::string& procedure /*= "main"*/)
{
    assert(std::find_if(m_stageInfos.begin(), m_stageInfos.end(), [&shader](const auto& info) {
               return info.stage == (VkShaderStageFlagBits)shader.stage();
           }) == m_stageInfos.end());

    VkPipelineShaderStageCreateInfo stageCreateInfo = {};

    stageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageCreateInfo.stage  = (VkShaderStageFlagBits)shader.stage();
    stageCreateInfo.module = detail::getVkHandle(shader);
    assert(shader.hasProcedure(procedure));
    assert(m_stageInfos.size() < m_stageProcedures.size());
    auto& name = m_stageProcedures[m_stageInfos.size()] = procedure;
    stageCreateInfo.pName                               = name.c_str();

    m_stageInfos.push_back(stageCreateInfo);
}

inline void ShaderPipeline::removeStage(ShaderStage stage)
{
    auto found = std::find_if(m_stageInfos.begin(), m_stageInfos.end(),
                              [stage](const auto& info) { return info.stage == (VkShaderStageFlagBits)stage; });
    if (found == m_stageInfos.end())
        return;

    const size_t end  = m_stageInfos.size();
    size_t       from = util::index_of(m_stageInfos, found);
    m_stageInfos.erase(found);

    std::move(m_stageProcedures.begin() + from + 1, m_stageProcedures.begin() + end, m_stageProcedures.begin() + from);
    // update address of the procedure name
    size_t i = from;
    std::for_each(m_stageInfos.begin() + from, m_stageInfos.end(),
                  [&i, this](auto& info) { info.pName = m_stageProcedures[i++].c_str(); });
}
}  // namespace ri
