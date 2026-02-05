cmake_minimum_required(VERSION 3.10)
project(fxed)

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0067 NEW)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fno-omit-frame-pointer -O3 -std=c++20")

file(GLOB SOURCE_FILES src/*.cpp)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug build")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")
else()
    message("Release build")
    set(CMAKE_LINKER_FLAGS  "${CMAKE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static-libc++ --static -s")


	file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shadercache/*.spv")
	find_package(Python3 REQUIRED COMPONENTS Interpreter)
	add_custom_command(
	    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shader_registry.cpp
	    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_shader_registry.py 
	            > ${CMAKE_CURRENT_BINARY_DIR}/shader_registry.cpp
	    DEPENDS ${SHADER_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/gen_shader_registry.py
	    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	    COMMENT "Generating shader registry..."
	)
	set(SOURCE_FILES 
		${SOURCE_FILES}
		${CMAKE_CURRENT_BINARY_DIR}/shader_registry.cpp
	)
endif()



if (UNIX)
    find_package (Vulkan REQUIRED dxc)
else()
    find_package (Vulkan REQUIRED)
endif()

add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/msdf-atlas-gen EXCLUDE_FROM_ALL)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
if (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::dxc_lib fontconfig)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE $ENV{VULKAN_SDK}/Lib/dxcompiler.lib)
endif ()
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)
target_link_libraries(${PROJECT_NAME} PRIVATE msdf-atlas-gen)

# On Windows, explicitly link FreeType dependencies after msdf-atlas-gen
# This is needed for static linking where dependencies aren't automatically propagated
if (NOT UNIX)
    # Force static linking by explicitly wrapping libraries with -Bstatic/-Bdynamic
    target_link_libraries(${PROJECT_NAME} PRIVATE freetype png bz2)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ../
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    # path to the root directory of the project
	#PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
	DBG_LOG_LEVEL=-1
)

