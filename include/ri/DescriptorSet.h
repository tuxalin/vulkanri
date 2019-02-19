#pragma once

#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/ComputePipeline.h>
#include <ri/RenderPipeline.h>
#include <ri/Texture.h>
#include <ri/Types.h>

namespace ri
{
struct DescriptorSetParams
{
    enum Mode
    {
        eBuffer = 0,
        eTexelBuffer,
        eTexture
    };
    enum TextureType
    {
        eCombinedSampler = DescriptorType::eCombinedSampler,
        eImage           = DescriptorType::eImage,
        eSampledImage    = DescriptorType::eSampledImage,
    };

    struct WriteInfo
    {
        union {
            const Buffer*  buffer;
            const Texture* texture;
        };
        uint32_t       offset, size;
        uint32_t       binding;
        DescriptorType type;

        WriteInfo(uint32_t binding, const Buffer* buffer, uint32_t offset, uint32_t size);
        WriteInfo(uint32_t binding, const Buffer* buffer, DescriptorType type);
        WriteInfo(uint32_t binding, const Texture* texture, TextureType type = eCombinedSampler);

        const Mode mode() const;

    private:
        Mode m_mode;
    };

    DescriptorSetParams();
    DescriptorSetParams(std::initializer_list<WriteInfo> infos);
    DescriptorSetParams(uint32_t binding, const Buffer* buffer, uint32_t offset, uint32_t size);
    DescriptorSetParams(uint32_t binding, const Texture* texture);

    template <class... Args>
    void add(Args&&... args);

    std::vector<WriteInfo> infos;
};

class DescriptorSet : public RenderObject<VkDescriptorSet>
{
public:
    DescriptorSet();

    template <int InfoCount>
    void update(const DescriptorSetParams& params);

    void bind(CommandBuffer& buffer, const RenderPipeline& pipeline) const;
    void bind(CommandBuffer& buffer, const ComputePipeline& pipeline) const;

    /// Batch call for setting multiple descriptors.
    ///@note Preferred over individual calls.
    template <int InfoCount, int Count>
    static void update(const DescriptorSet* (&descriptors)[Count],
                       const DescriptorSetParams (&descriptorParams)[Count]);
    /// Batch call for binding multiple descriptors.
    ///@note Preferred over individual calls.
    template <int Count>
    static void bind(CommandBuffer& buffer, const RenderPipeline& pipeline, const DescriptorSet* (&descriptors)[Count]);

private:
    struct DescriptorInfo
    {
        union {
            VkDescriptorBufferInfo buffer;
            VkDescriptorImageInfo  image;
        };
    };

private:
    DescriptorSet(VkDevice device, VkDescriptorSet handle)
        : RenderObject<VkDescriptorSet>(handle)
        , m_device(device)
    {
    }

    static void setInfos(const DescriptorSetParams::WriteInfo& params, VkDescriptorSet descriptor,  //
                         DescriptorInfo& info, VkWriteDescriptorSet& descriptorWrite)
    {
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        switch (params.mode())
        {
            case DescriptorSetParams::eBuffer:
            {
                auto& bufferInfo                 = info.buffer;
                bufferInfo.buffer                = detail::getVkHandle(*params.buffer);
                bufferInfo.offset                = params.offset;
                bufferInfo.range                 = params.size;
                descriptorWrite.pBufferInfo      = &bufferInfo;
                descriptorWrite.pImageInfo       = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;
                break;
            }
            case DescriptorSetParams::eTexture:
            {
                auto&       imageInfo            = info.image;
                const auto& textureInfo          = detail::getTextureDescriptorInfo(*params.texture);
                imageInfo.imageLayout            = textureInfo.layout;
                imageInfo.imageView              = textureInfo.imageView;
                imageInfo.sampler                = textureInfo.sampler;
                descriptorWrite.pImageInfo       = &imageInfo;
                descriptorWrite.pBufferInfo      = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;
                break;
            }
            case DescriptorSetParams::eTexelBuffer:
            {
                // TODO: add support
                assert(false);
                VkBufferView bufferView;
                descriptorWrite.pTexelBufferView = &bufferView;
                descriptorWrite.pImageInfo       = nullptr;
                descriptorWrite.pBufferInfo      = nullptr;
            }
            default:
                break;
        }

        descriptorWrite.dstSet     = descriptor;
        descriptorWrite.dstBinding = params.binding;
        // TODO: expose array update
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = (VkDescriptorType)params.type;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pNext           = nullptr;
    }

    VkDevice m_device = VK_NULL_HANDLE;

    friend class DescriptorPool;  // DescriptorSet can be created only from a pool
};

inline DescriptorSet::DescriptorSet() {}

template <int InfoCount>
inline void DescriptorSet::update(const DescriptorSetParams& params)
{
    assert(m_handle);

    DescriptorInfo       descriptorInfos[InfoCount];
    VkWriteDescriptorSet descriptorWriteInfos[InfoCount];

    size_t i = 0;
    for (const auto& writeInfo : params.infos)
    {
        assert(i < InfoCount);
        setInfos(writeInfo, m_handle, descriptorInfos[i], descriptorWriteInfos[i]);
        ++i;
    }

    vkUpdateDescriptorSets(m_device, i, descriptorWriteInfos, 0, nullptr);
}

inline void DescriptorSet::bind(CommandBuffer& buffer, const RenderPipeline& pipeline) const
{
    assert(m_handle);
    vkCmdBindDescriptorSets(detail::getVkHandle(buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
                            detail::getPipelineLayout(pipeline), 0, 1, &m_handle, 0, nullptr);
}

inline void DescriptorSet::bind(CommandBuffer& buffer, const ComputePipeline& pipeline) const
{
    assert(m_handle);
    vkCmdBindDescriptorSets(detail::getVkHandle(buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
                            detail::getPipelineLayout(pipeline), 0, 1, &m_handle, 0, nullptr);
}

template <int InfoCount, int Count>
static void DescriptorSet::update(const DescriptorSet* (&descriptors)[Count],
                                  const DescriptorSetParams (&descriptorParams)[Count])
{
    DescriptorInfo       descriptorInfos[InfoCount];
    VkWriteDescriptorSet descriptorWriteInfos[InfoCount];

    const auto device = descriptors[0]->m_device;
    size_t     j      = 0;
    for (int i = 0; i < Count; ++i)
    {
        auto descriptor = descriptors[i];
        assert(descriptor);
        assert(descriptor->m_handle);

        const auto& params = descriptorParams[i];
        for (const auto& writeInfo : params.infos)
        {
            assert(j < InfoCount);
            setInfos(writeInfo, descriptor->m_handle, descriptorInfos[j++], descriptorWriteInfos[j++]);
        }
    }
    vkUpdateDescriptorSets(device, j, descriptorWriteInfos, 0, nullptr);
}

template <int Count>
void DescriptorSet::bind(CommandBuffer& buffer, const RenderPipeline& pipeline,
                         const DescriptorSet* (&descriptors)[Count])
{
    VkDescriptorSet handles[Count];

    for (int i = 0; i < Count; ++i)
    {
        auto descriptor = descriptors[i];
        assert(descriptor);
        assert(descriptor->m_handle);
        handles[i] = descriptor->m_handle;
    }

    vkCmdBindDescriptorSets(detail::getVkHandle(buffer), m_bindPoint, detail::getPipelineLayout(pipeline), 0, Count,
                            handles, 0, nullptr);
}

//

inline DescriptorSetParams::DescriptorSetParams() {}

inline DescriptorSetParams::DescriptorSetParams(std::initializer_list<WriteInfo> infos)
    : infos(infos)
{
}

inline DescriptorSetParams::DescriptorSetParams(uint32_t binding, const Buffer* buffer, uint32_t offset, uint32_t size)
    : infos(1, WriteInfo(binding, buffer, offset, size))
{
}

inline DescriptorSetParams::DescriptorSetParams(uint32_t binding, const Texture* texture)
    : infos(1, WriteInfo(binding, texture))
{
}

template <class... Args>
void DescriptorSetParams::add(Args&&... args)
{
    infos.emplace_back(std::forward<Args>(args)...);
}

inline DescriptorSetParams::WriteInfo::WriteInfo(uint32_t binding, const Buffer* buffer, uint32_t offset, uint32_t size)
    : buffer(buffer)
    , offset(offset)
    , size(size)
    , binding(binding)
    , m_mode(eBuffer)
    , type(DescriptorType::eUniformBuffer)
{
}

inline DescriptorSetParams::WriteInfo::WriteInfo(uint32_t binding, const Buffer* buffer, DescriptorType type)
    : buffer(buffer)
    , offset(0)
    , size(buffer ? buffer->bytes() : 0)
    , binding(binding)
    , m_mode(eBuffer)
    , type(type)
{
}

inline DescriptorSetParams::WriteInfo::WriteInfo(uint32_t binding, const Texture* texture,
                                                 TextureType type /*= eCombinedSampler*/)
    : texture(texture)
    , binding(binding)
    , m_mode(eTexture)
    , type(static_cast<DescriptorType>(type))
{
}

inline const DescriptorSetParams::Mode DescriptorSetParams::WriteInfo::mode() const
{
    return m_mode;
}

}  // namespace ri
