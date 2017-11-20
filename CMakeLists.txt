
cmake_minimum_required(VERSION 3.2)
project(vulkanri C CXX)

################################################################################################
# CMake helpers
################################################################################################

function(POPULATE_SOURCE_FOLDER allSources projectSourceDir sourceFolder virtualFolder)
    ## create the group path where sources should be stored
    string(REPLACE "\\" "\\\\" SOURCE_GROUP_PATH ${sourceFolder})
    string(REPLACE "/" "\\\\" SOURCE_GROUP_PATH ${SOURCE_GROUP_PATH})

    ## get all the source files from the folder
    AUX_SOURCE_DIRECTORY("${projectSourceDir}/${sourceFolder}" LOCAL_SOURCES_FILES)

    ## get all the headers from the folder
    file (GLOB LOCAL_HEADERS_FILES "${projectSourceDir}/${sourceFolder}/*.h" "${projectSourceDir}/${sourceFolder}/*.hpp")

    ## add the files to the source tree
    set(${allSources}
        ${${allSources}}
        ${LOCAL_SOURCES_FILES}
        ${LOCAL_HEADERS_FILES}
        PARENT_SCOPE)

    ## create the source group for the files
    if (NOT "${virtualFolder}" STREQUAL "")
        SET(SOURCE_GROUP_PATH "${virtualFolder}\\\\${SOURCE_GROUP_PATH}")
    endif(NOT "${virtualFolder}" STREQUAL "")
    source_group(${SOURCE_GROUP_PATH} FILES ${LOCAL_SOURCES_FILES} ${LOCAL_HEADERS_FILES})
endfunction(POPULATE_SOURCE_FOLDER)

function(POPULATE_SOURCE_FOLDER_RECURSIVE allSources projectSourceDir virtualFolder)

    ## recursive get all files
    set(ALL_LOCAL_FILES "")
    file (GLOB_RECURSE ALL_LOCAL_FILES "${projectSourceDir}/*.h" "${projectSourceDir}/*.hpp" "${projectSourceDir}/*.cpp" "${projectSourceDir}/*.c")

    SET(DIR_LIST "")
    foreach (CURRENT_FILE ${ALL_LOCAL_FILES})
        ## remove the projectSourceDir from found paths
        string(LENGTH ${projectSourceDir} PROJECT_SOURCE_DIR_LENGTH)
        math(EXPR PROJECT_SOURCE_DIR_LENGTH "${PROJECT_SOURCE_DIR_LENGTH} + 1")
        string(SUBSTRING ${CURRENT_FILE} ${PROJECT_SOURCE_DIR_LENGTH} -1 PROCESS_PATH)

        ## get the path of the file
        GET_FILENAME_COMPONENT(DIR_PATH ${PROCESS_PATH} PATH)
        # message("${DIR_PATH} ${PROCESS_PATH}")
        SET(DIR_LIST ${DIR_LIST} ${DIR_PATH})
    endforeach(CURRENT_FILE)

    ## remove duplicate directories
    LIST(REMOVE_DUPLICATES DIR_LIST)

    ## create the soruce tree frome each directory
    SET(newSources "")
    foreach(SCANNED_PATH ${DIR_LIST})
        POPULATE_SOURCE_FOLDER(newSources "${projectSourceDir}" "${SCANNED_PATH}" "${virtualFolder}")
    endforeach(SCANNED_PATH)

    ## set up the allSources variables
    set(${allSources}
        ${${allSources}}
        ${newSources}
        PARENT_SCOPE)

endfunction(POPULATE_SOURCE_FOLDER_RECURSIVE)

################################################################################################
# Header files
################################################################################################

# util
file(GLOB util_HDR
	${PROJECT_SOURCE_DIR}/include/util/*.h)

# render interface
file(GLOB render_inteface_HDR
	${PROJECT_SOURCE_DIR}/include/ri/*.h
	${PROJECT_SOURCE_DIR}/include/ri/internal/*.h)

################################################################################################
# Source files
################################################################################################

# render interface
file(GLOB render_inteface_SRC
	${PROJECT_SOURCE_DIR}/src/ri/*.h
	${PROJECT_SOURCE_DIR}/src/ri/*.cpp)

SET(ALL_ENGINE_SOURCE_FILES "")
POPULATE_SOURCE_FOLDER_RECURSIVE(ALL_ENGINE_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/" "Source Files")
POPULATE_SOURCE_FOLDER_RECURSIVE(ALL_ENGINE_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include" "Header Files")

################################################################################################
# Compiler flags and linker
################################################################################################

include_directories(
	${PROJECT_SOURCE_DIR}/include/
	${Vulkan_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR})

add_library(vulkanri STATIC 
	${util_HDR}
	${render_inteface_HDR}
	${render_inteface_SRC}
    )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
