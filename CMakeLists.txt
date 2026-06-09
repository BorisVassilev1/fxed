cmake_minimum_required(VERSION 3.10)
project(fxed)

#set(CMAKE_CXX_COMPILER /usr/bin/clang++)
#set(CMAKE_C_COMPILER /usr/bin/clang)

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0067 NEW)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O3 -ffunction-sections -fdata-sections")

file(GLOB SOURCE_FILES src/*.cpp)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
	message("Debug build")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")
else()
	message("Release build")
	add_link_options(-Wl,--gc-sections)
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
set(FT_DISABLE_PNG OFF CACHE BOOL "" FORCE)
set(FT_REQUIRE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON)
set(FT_DISABLE_BROTLI ON)
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_TESTS  OFF CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS OFF)
#set(ZLIB_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)

add_subdirectory(lib/libpng EXCLUDE_FROM_ALL)
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/vk-bootstrap EXCLUDE_FROM_ALL)

set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)

# Make FreeType's find_package(PNG) resolve to our png_static
set(PNG_PNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng CACHE PATH "" FORCE)
set(PNG_LIBRARY png_static CACHE STRING "" FORCE)
set(PNG_FOUND TRUE CACHE BOOL "" FORCE)

if(WIN32)
    set(_png_lib "${CMAKE_CURRENT_BINARY_DIR}/lib/libpng/libpng18_static.lib")
else()
    set(_png_lib "${CMAKE_CURRENT_BINARY_DIR}/lib/libpng/libpng18.a")
endif()
if(NOT TARGET PNG::PNG)
    add_library(PNG::PNG STATIC IMPORTED GLOBAL)
    set_target_properties(PNG::PNG PROPERTIES
        IMPORTED_LOCATION             "${_png_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng;${CMAKE_CURRENT_BINARY_DIR}/lib/libpng"
		INTERFACE_LINK_LIBRARIES      "png_static;m"
    )
    add_dependencies(PNG::PNG png_static)
endif()

get_target_property(_png_libs png_static INTERFACE_LINK_LIBRARIES)
message(STATUS "png_static INTERFACE_LINK_LIBRARIES: ${_png_libs}")

add_subdirectory(lib/freetype EXCLUDE_FROM_ALL)

## Create the Freetype::Freetype alias that msdfgen expects
if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

set(FREETYPE_FOUND TRUE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	get_target_property(_freetype_debug_postfix freetype DEBUG_POSTFIX)
	if(_freetype_debug_postfix STREQUAL "_freetype_debug_postfix-NOTFOUND")
		set(_freetype_debug_postfix "")
	endif()
	set(FREETYPE_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/freetype/${CMAKE_STATIC_LIBRARY_PREFIX}freetype${_freetype_debug_postfix}${CMAKE_STATIC_LIBRARY_SUFFIX}")
else()
	set(FREETYPE_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/freetype/${CMAKE_STATIC_LIBRARY_PREFIX}freetype${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()
set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype/include")
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
target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
target_link_libraries(${PROJECT_NAME} PRIVATE png_static)

set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	DBG_LOG_LEVEL=-1
)
#target_link_options(${PROJECT_NAME} PRIVATE -Wl,--verbose)
