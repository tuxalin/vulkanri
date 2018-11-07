#pragma once

#include "Config.h"
#include <util/safe_enum.h>

namespace ri
{
SAFE_ENUM_DECLARE(DeviceOperation, eGraphics = 0, eTransfer, eCompute);

SAFE_ENUM_DECLARE(DeviceFeature,
                  eFloat64 = 0,
                  eGeometryShader,
                  eTesselationShader,
                  eSwapchain,
                  eAnisotropy,
                  eSampleRateShading,
                  eWireframe);

SAFE_ENUM_DECLARE(ShaderStage,
                  eVertex                 = VK_SHADER_STAGE_VERTEX_BIT,
                  eGeometry               = VK_SHADER_STAGE_GEOMETRY_BIT,
                  eTessellationControl    = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                  eTessellationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                  eFragment               = VK_SHADER_STAGE_FRAGMENT_BIT,
                  eCompute                = VK_SHADER_STAGE_COMPUTE_BIT,
                  eVertexFragment         = eVertex | eFragment,
                  eAllGraphics            = VK_SHADER_STAGE_ALL_GRAPHICS);

SAFE_ENUM_DECLARE(PrimitiveTopology,
                  eTriangles             = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                  eTriangleStrip         = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                  eLines                 = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                  eLineStrip             = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
                  ePoints                = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                  eLineListAdjaceny      = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
                  eLineStripAdjaceny     = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
                  eTriangleListAdjaceny  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
                  eTriangleStripAdjaceny = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
                  ePatchList             = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);

SAFE_ENUM_DECLARE(CullMode,
                  eNone  = VK_CULL_MODE_NONE,  //
                  eBack  = VK_CULL_MODE_BACK_BIT,
                  eFront = VK_CULL_MODE_FRONT_BIT);

SAFE_ENUM_DECLARE(PolygonMode,
                  eNormal    = VK_POLYGON_MODE_FILL,  //
                  eWireframe = VK_POLYGON_MODE_LINE);

SAFE_ENUM_DECLARE(BlendFactor,
                  eZero                     = VK_BLEND_FACTOR_ZERO,
                  eOne                      = VK_BLEND_FACTOR_ONE,
                  eSrc_Color                = VK_BLEND_FACTOR_SRC_COLOR,
                  eOne_Minus_Src_Color      = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
                  eDst_Color                = VK_BLEND_FACTOR_DST_COLOR,
                  eOne_Minus_Dst_Color      = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
                  eSrc_Alpha                = VK_BLEND_FACTOR_SRC_ALPHA,
                  eOne_Minus_Src_Alpha      = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                  eDst_Alpha                = VK_BLEND_FACTOR_DST_ALPHA,
                  eOne_Minus_Dst_Alpha      = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
                  eConstant_Color           = VK_BLEND_FACTOR_CONSTANT_COLOR,
                  eOne_Minus_Constant_Color = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
                  eConstant_Alpha           = VK_BLEND_FACTOR_CONSTANT_ALPHA,
                  eOne_Minus_Constant_Alpha = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
                  eSrc_Alpha_Saturate       = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);

SAFE_ENUM_DECLARE(BlendOperation,
                  eAdd         = VK_BLEND_OP_ADD,
                  eSubtract    = VK_BLEND_OP_SUBTRACT,
                  eRevSubtract = VK_BLEND_OP_REVERSE_SUBTRACT,
                  eMin         = VK_BLEND_OP_MIN,
                  eMax         = VK_BLEND_OP_MAX,
                  eZero        = VK_BLEND_OP_ZERO_EXT);

SAFE_ENUM_DECLARE(CompareOperation,
                  eNever          = VK_COMPARE_OP_NEVER,
                  eLess           = VK_COMPARE_OP_LESS,
                  eEqual          = VK_COMPARE_OP_EQUAL,
                  eLessOrEqual    = VK_COMPARE_OP_LESS_OR_EQUAL,
                  eGreater        = VK_COMPARE_OP_GREATER,
                  eNotEqual       = VK_COMPARE_OP_NOT_EQUAL,
                  eGreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
                  eAlways         = VK_COMPARE_OP_ALWAYS);

SAFE_ENUM_DECLARE(DynamicState,
                  eDepthBias          = VK_DYNAMIC_STATE_DEPTH_BIAS,
                  eStencilCompareMask = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
                  eStencilWriteMask   = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
                  eStencilReference   = VK_DYNAMIC_STATE_STENCIL_REFERENCE,
                  eLineWidth          = VK_DYNAMIC_STATE_LINE_WIDTH,
                  eViewport           = VK_DYNAMIC_STATE_VIEWPORT,
                  eScissor            = VK_DYNAMIC_STATE_SCISSOR);

SAFE_ENUM_DECLARE(AttributeFormat,
                  eHalfFloat  = VK_FORMAT_R16_SFLOAT,           //
                  eHalfFloat2 = VK_FORMAT_R16G16_SFLOAT,        //
                  eHalfFloat3 = VK_FORMAT_R16G16B16_SFLOAT,     //
                  eHalfFloat4 = VK_FORMAT_R16G16B16A16_SFLOAT,  //
                  eFloat      = VK_FORMAT_R32_SFLOAT,           //
                  eFloat2     = VK_FORMAT_R32G32_SFLOAT,        //
                  eFloat3     = VK_FORMAT_R32G32B32_SFLOAT,     //
                  eFloat4     = VK_FORMAT_R32G32B32A32_SFLOAT,  //
                  eDouble     = VK_FORMAT_R64_SFLOAT,           //
                  eDouble2    = VK_FORMAT_R64G64_SFLOAT,        //
                  eDouble3    = VK_FORMAT_R64G64B64_SFLOAT,     //
                  eDouble4    = VK_FORMAT_R64G64B64A64_SFLOAT,  //
                  eShort      = VK_FORMAT_R16_UINT,             //
                  eShort2     = VK_FORMAT_R16G16_UINT,          //
                  eShort3     = VK_FORMAT_R16G16B16_UINT,       //
                  eShort4     = VK_FORMAT_R16G16B16A16_UINT);

SAFE_ENUM_DECLARE(IndexType,
                  eInt16 = VK_INDEX_TYPE_UINT16,  //
                  eInt32 = VK_INDEX_TYPE_UINT32);

SAFE_ENUM_DECLARE(BufferUsageFlags,                                                  //
                  eSrc      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,                      //
                  eDst      = VK_BUFFER_USAGE_TRANSFER_DST_BIT,                      //
                  eUniform  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                    //
                  eIndex    = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,                      //
                  eVertex   = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,                     //
                  eIndirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,                   //
                  eIndexSrc = eIndex | eSrc, eIndexDst = eIndex | eDst,              //
                  eVertexSrc = eVertex | eSrc, eVertexDst = eVertex | eDst,          //
                  eIndirectSrc = eIndirect | eSrc, eIndirectDst = eIndirect | eDst,  //
                  eUniformtSrc = eUniform | eSrc, eUniformDst = eUniform | eDst);

SAFE_ENUM_DECLARE(TextureType,
                  e1D      = VK_IMAGE_VIEW_TYPE_1D,
                  e2D      = VK_IMAGE_VIEW_TYPE_2D,
                  e3D      = VK_IMAGE_VIEW_TYPE_3D,
                  eCube    = VK_IMAGE_VIEW_TYPE_CUBE,
                  eArray1D = VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                  eArray2D = VK_IMAGE_VIEW_TYPE_2D_ARRAY);

SAFE_ENUM_DECLARE(TextureLayoutType,                                  //
                  eUndefined            = VK_IMAGE_LAYOUT_UNDEFINED,  //
                  eGeneral              = VK_IMAGE_LAYOUT_GENERAL,
                  eColorOptimal         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                  eDepthStencilOptimal  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                  eDepthStencilReadOnly = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                  eShaderReadOnly       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  eTransferSrcOptimal   = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  eTransferDstOptimal   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  ePresentSrc           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  eSharedPresentSrc     = VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
                  ePreinitialized       = VK_IMAGE_LAYOUT_PREINITIALIZED);

SAFE_ENUM_DECLARE(TextureUsageFlags,
                  eSrc          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                  eDst          = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                  eSampled      = VK_IMAGE_USAGE_SAMPLED_BIT,
                  eStorage      = VK_IMAGE_USAGE_STORAGE_BIT,
                  eColor        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  eDepthStencil = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  eTransient    = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);

SAFE_ENUM_DECLARE(TextureTiling,                       //
                  eOptimal = VK_IMAGE_TILING_OPTIMAL,  //
                  eLinear  = VK_IMAGE_TILING_LINEAR);

SAFE_ENUM_DECLARE(DescriptorType,
                  eUniformBuffer        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                  eUniformBufferDynamic = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                  eSampler              = VK_DESCRIPTOR_TYPE_SAMPLER,
                  eCombinedSampler      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                  eSampledImage         = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                  eImage                = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                  eTexelBuffer          = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                  eStorageBuffer        = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                  eStorageBufferDynamic = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);

SAFE_ENUM_DECLARE(ColorFormat,
                  eRed             = VK_FORMAT_R8G8_UNORM,           //
                  eRGB565          = VK_FORMAT_R5G6B5_UNORM_PACK16,  //
                  eBGRA            = VK_FORMAT_B8G8R8A8_UNORM,       //
                  eRGBA            = VK_FORMAT_R8G8B8A8_UNORM,       //
                  eDepth32         = VK_FORMAT_D32_SFLOAT,           //
                  eDepth24Stencil8 = VK_FORMAT_D32_SFLOAT_S8_UINT,   //
                  eDepth32Stencil8 = VK_FORMAT_D24_UNORM_S8_UINT,    //
                  eUndefined       = VK_FORMAT_UNDEFINED);

SAFE_ENUM_DECLARE(ComponentSwizzle,                           //
                  eIdentity = VK_COMPONENT_SWIZZLE_IDENTITY,  //
                  eZero     = VK_COMPONENT_SWIZZLE_ZERO,      //
                  eOne      = VK_COMPONENT_SWIZZLE_ONE,       //
                  eRed      = VK_COMPONENT_SWIZZLE_R,         //
                  eGreen    = VK_COMPONENT_SWIZZLE_G,         //
                  eBlue     = VK_COMPONENT_SWIZZLE_B,         //
                  eAlpha    = VK_COMPONENT_SWIZZLE_A);

SAFE_ENUM_DECLARE(ReportLevel,
                  eError       = VK_DEBUG_REPORT_ERROR_BIT_EXT,                            //
                  eWarning     = eError | VK_DEBUG_REPORT_WARNING_BIT_EXT,                 //
                  eInfo        = eError | eWarning | VK_DEBUG_REPORT_INFORMATION_BIT_EXT,  //
                  ePerformance = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,              //
                  eDebug       = eInfo | VK_DEBUG_REPORT_DEBUG_BIT_EXT);

SAFE_ENUM_DECLARE(DeviceCommandHint,
                  // Hint that  device command buffers are prerecorded.
                  eRecorded = 0,
                  // Hint that device command buffers are rerecorded with new commands very often.
                  eTransient = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

SAFE_ENUM_DECLARE(RecordFlags,
                  // Specifies that each recording of the command buffer will only be submitted once, and the command
                  // buffer will be reset and recorded again between each submission.
                  eOneTime = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                  // This is a secondary command buffer that will be entirely within a single render pass.
                  eSecondary = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
                  // The command buffer can be resubmitted while it is also already pending execution.
                  eResubmit = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

SAFE_ENUM_DECLARE(
    PresentMode,
    // Images submitted are transferred to the screen right away, may result in tearing.
    eImmediate = VK_PRESENT_MODE_IMMEDIATE_KHR,
    // The swap chain is a queue where the display takes an image from the front of the queue when the display
    // is refreshed and the clients inserts rendered images at the back of the queue.
    // If the queue is full then the client has to wait, also guaranteed to be available on any platform.
    eNormal = VK_PRESENT_MODE_FIFO_KHR,
    // Doesn't block the client if the queue is full, the images that are already queued are simply replaced with the
    // newer ones.
    eMailbox = VK_PRESENT_MODE_MAILBOX_KHR);

typedef VkDescriptorSetLayout   DescriptorSetLayout;
typedef VkImageFormatProperties TextureProperties;
typedef VkStencilOpState        StencilOpState;

struct DeviceProperties : VkPhysicalDeviceProperties
{
    uint32_t getMaxSamples() const;
};

union ClearColorValue  //
{
    float    float32[4];
    int32_t  int32[4];
    uint32_t uint32[4];
};

struct ClearDepthStencilValue
{
    float    depth;
    uint32_t stencil;
};

union ClearValue  //
{
    ClearColorValue        color;
    ClearDepthStencilValue depthStencil;
};

struct SurfaceCreateParams
{
    SurfaceCreateParams() {}
#if RI_PLATFORM == RI_PLATFORM_WINDOWS
    int       flags     = 0;
    HINSTANCE hinstance = nullptr;
    HWND      hwnd      = nullptr;
#elif RI_PLATFORM == RI_PLATFORM_GLFW
    SurfaceCreateParams(GLFWwindow* window)
        : window(window)
    {
    }

    GLFWwindow* window = nullptr;
#endif

    enum DepthBufferType
    {
        eNone            = ColorFormat::eUndefined,
        eDepth32         = ColorFormat::eDepth32,
        eDepth24Stencil8 = ColorFormat::eDepth24Stencil8,
        eDepth32Stencil8 = ColorFormat::eDepth32Stencil8,
    };

    DepthBufferType depthBufferType = eNone;
    uint32_t        msaaSamples     = 1;
};

inline uint32_t DeviceProperties::getMaxSamples() const
{
    VkSampleCountFlags counts = std::min(limits.framebufferColorSampleCounts, limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

}  // namespace ri
