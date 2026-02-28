cmake_minimum_required(VERSION 3.10)
project(fxed)

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0067 NEW)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fno-omit-frame-pointer -O3")

file(GLOB SOURCE_FILES src/*.cpp)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug build")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")
else()
    message("Release build")
    add_link_options(-static-libgcc -static-libstdc++)

    file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shadercache/*.spv")
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

set(FT_DISABLE_BZIP2 ON)
set(FT_DISABLE_HARFBUZZ ON)
#set(FT_DISABLE_PNG ON)
set(FT_DISABLE_ZLIB ON)
set(FT_DISABLE_BROTLI ON)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/vk-bootstrap EXCLUDE_FROM_ALL)

add_subdirectory(lib/freetype EXCLUDE_FROM_ALL)
# Create the Freetype::Freetype alias that msdfgen expects
if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)
set(FREETYPE_FOUND TRUE CACHE BOOL "" FORCE)
set(FREETYPE_LIBRARY freetype CACHE STRING "" FORCE)
set(FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype/include)
set(MSDFGEN_DISABLE_PNG ON)
set(MSDFGEN_CORE_ONLY ON)
add_subdirectory(lib/msdf-atlas-gen EXCLUDE_FROM_ALL)
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH ON)

set(BUILD_SHARED_LIBS ON)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap::vk-bootstrap)

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE fontconfig)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (UNIX)
        target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::dxc_lib)
    else ()
        target_link_libraries(${PROJECT_NAME} PRIVATE $ENV{VULKAN_SDK}/Lib/dxcompiler.lib)
    endif ()
endif()
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE lib/)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)
target_link_libraries(${PROJECT_NAME} PRIVATE msdf-atlas-gen)
target_link_libraries(${PROJECT_NAME} PRIVATE freetype)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vk_convert.hpp
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_vk_convert.py
			> ${CMAKE_CURRENT_BINARY_DIR}/vk_convert.hpp
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gen_vk_convert.py
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Generating Vulkan conversion utilities..."
)
message(${CMAKE_CURRENT_BINARY_DIR}/vk_convert.hpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	"VK_CONVERT_HEADER=\"${CMAKE_CURRENT_BINARY_DIR}/vk_convert.hpp\""
)
add_custom_target(generate_vk_convert DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vk_convert.hpp)
add_dependencies(${PROJECT_NAME} generate_vk_convert)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ../
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
 DBG_LOG_LEVEL=-1
)
