cmake_minimum_required(VERSION 3.10)
project(fxed)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCE_FILES src/*.cpp)


if (UNIX)
    find_package (Vulkan REQUIRED dxc)
else()
    find_package (Vulkan REQUIRED)
endif()


add_subdirectory(lib/glfw)
add_subdirectory(lib/glm)


add_executable(${PROJECT_NAME} ${SOURCE_FILES} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
if (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::dxc_lib)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ../
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    # path to the root directory of the project
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
	DBG_LOG_LEVEL=-1
)
