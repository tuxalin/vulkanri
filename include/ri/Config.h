#pragma once

#include <vulkan/vulkan.h>

#include "internal/ri_internal.h"

#define RI_PLATFORM_GLFW 1

#ifndef RI_PLATFORM
#define RI_PLATFORM RI_PLATFORM_GLFW
#endif  // ! RI_PLATFORM

#if RI_PLATFORM == RI_PLATFORM_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
