
cmake_minimum_required(VERSION 3.7)
project(examples)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT DEFINED VULKAN_RI_DIR)
set (VULKAN_RI_DIR ${PROJECT_SOURCE_DIR}/../.)
endif()

set(GLFW_USE_STATIC_LIBS 1)
find_package(GLFW REQUIRED)
find_package(Vulkan)

if(WIN32)
	if (NOT Vulkan_FOUND)
			error("Vulkan not found!")
	endif()
endif()

add_subdirectory(${VULKAN_RI_DIR} vulkanri)

include_directories(
	${VULKAN_RI_DIR}/include/
	${Vulkan_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR})

set (EXAMPLES_LIBRARIES vulkanri ${Vulkan_LIBRARY} ${GLFW_LIBRARIES})
	
add_executable(hello_world hello_world/main.cpp)
target_link_libraries(hello_world ${EXAMPLES_LIBRARIES})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
