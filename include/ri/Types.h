#pragma once

#include <util/safe_enum.h>

#include "Config.h"

namespace ri
{
SAFE_ENUM_DECLARE(DeviceOperations, eGraphics = 0, eTransfer, eCompute);

SAFE_ENUM_DECLARE(DeviceFeatures, eFloat64 = 0, eGeometryShader, eSwapchain);

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

SAFE_ENUM_DECLARE(ShaderStage,
                  eVertex   = VK_SHADER_STAGE_VERTEX_BIT,
                  eGeometry = VK_SHADER_STAGE_GEOMETRY_BIT,
                  eFragment = VK_SHADER_STAGE_FRAGMENT_BIT,
                  eCompute  = VK_SHADER_STAGE_COMPUTE_BIT);

SAFE_ENUM_DECLARE(PrimitiveTopology,
                  eTriangles     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,   //
                  eTriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,  //
                  eLines         = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,       //
                  eLineStrip     = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,      //
                  ePoints        = VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

SAFE_ENUM_DECLARE(CullMode,
                  eNone  = VK_CULL_MODE_NONE,      //
                  eBack  = VK_CULL_MODE_BACK_BIT,  //
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
                  eAdd         = VK_BLEND_OP_ADD,               //
                  eSubtract    = VK_BLEND_OP_SUBTRACT,          //
                  eRevSubtract = VK_BLEND_OP_REVERSE_SUBTRACT,  //
                  eMin         = VK_BLEND_OP_MIN,               //
                  eMax         = VK_BLEND_OP_MAX,               //
                  eZero        = VK_BLEND_OP_ZERO_EXT);

SAFE_ENUM_DECLARE(ColorFormat,
                  eRed    = VK_FORMAT_R8G8_UNORM,           //
                  eRGB565 = VK_FORMAT_R5G6B5_UNORM_PACK16,  //
                  eBGRA   = VK_FORMAT_B8G8R8A8_UNORM,       //
                  eRGBA   = VK_FORMAT_R8G8B8A8_UNORM);
}  // namespace ri
