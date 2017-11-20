
cmake_minimum_required(VERSION 3.7)
project(examples)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT DEFINED VULKAN_RI_DIR)
set (VULKAN_RI_DIR ${PROJECT_SOURCE_DIR}/../.)
endif()

set(GLFW_USE_STATIC_LIBS 1)
find_package(GLFW REQUIRED)
find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
		error("Vulkan not found!")
endif()

add_subdirectory(${VULKAN_RI_DIR} vulkanri)

include_directories(
	${VULKAN_RI_DIR}/include/
	${Vulkan_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR})

set (EXAMPLES_LIBRARIES vulkanri ${Vulkan_LIBRARY} ${GLFW_LIBRARIES})
set (EXAMPLES_TARGETS "hello_world")

foreach( examples_target ${EXAMPLES_TARGETS} )
	add_executable(${examples_target} ${examples_target}/main.cpp)
	target_link_libraries(${examples_target} ${EXAMPLES_LIBRARIES})
	
	if(WIN32)
	add_custom_command(TARGET ${examples_target}
			PRE_BUILD 
			COMMAND cmd /C ${PROJECT_SOURCE_DIR}/compile_shaders.bat ${PROJECT_SOURCE_DIR}/
			COMMENT "Compiling shaders script"
			)
	else()
		error("TODO: compile shaders script for linux/macosx")
	endif()
endforeach()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
