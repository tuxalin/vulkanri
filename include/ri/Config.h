#pragma once

#define RI_PLATFORM_GLFW 1
#define RI_PLATFORM_WINDOWS 2

#if RI_PLATFORM == RI_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#elif RI_PLATFORM == RI_PLATFORM_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan.h>

#include "internal/ri_internal.h"
#include "internal/ri_internal_get_handle.h"
