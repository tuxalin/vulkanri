#pragma once

#include <ri/ApplicationInstance.h>
#include <ri/DeviceContext.h>

namespace ri
{ 
	namespace detail 
	{ 
		struct ApplicationInstance
		{
			VkInstance instance;
		};

		struct DeviceContext
		{
			using OperationIndices = std::array<int, (size_t)DeviceOperations::Count>;
			using OperationQueues = std::array<VkQueue, (size_t)DeviceOperations::Count>;

			const ApplicationInstance& m_instance;
			std::vector<DeviceOperations> m_requiredOperations;
			VkPhysicalDevice physicalDevice;
			VkDevice device;
			OperationQueues m_queues;
			OperationIndices m_queueIndices;
		};

		inline VkInstance getVkHandle(const ri::ApplicationInstance& appInstance)
		{
			static_assert(sizeof(ri::ApplicationInstance) == sizeof(ri::detail::ApplicationInstance), "INVALID_SIZES");
			return reinterpret_cast<const detail::ApplicationInstance&>(appInstance).instance;
		}

		inline VkPhysicalDevice getVkHandle(const ri::DeviceContext& appInstance)
		{
			static_assert(sizeof(ri::DeviceContext) == sizeof(ri::detail::DeviceContext), "INVALID_SIZES");
			return reinterpret_cast<const detail::DeviceContext&>(appInstance).physicalDevice;
		}

		inline VkDevice getVkLogicalHandle(const ri::DeviceContext& appInstance)
		{
			static_assert(sizeof(ri::DeviceContext) == sizeof(ri::detail::DeviceContext), "INVALID_SIZES");
			return reinterpret_cast<const detail::DeviceContext&>(appInstance).device;
		}
	} 
}
