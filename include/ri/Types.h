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

struct Size
{
    uint32_t width;
    uint32_t height;

    Size(uint32_t width, uint32_t height)
        : width(width)
        , height(height)
    {
    }
};
}
