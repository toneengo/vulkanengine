cmake_minimum_required(VERSION 3.12)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project("vulkanengine")

add_executable(vulkanengine)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "vulkanengine")

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vulkanengine PRIVATE glfw)
find_package(Vulkan REQUIRED)
target_link_libraries(vulkanengine PRIVATE Vulkan::Vulkan)
find_package(fmt REQUIRED)
target_link_libraries(vulkanengine PRIVATE fmt)

target_include_directories(vulkanengine PRIVATE "src/")
add_subdirectory(src)
set(env{CMAKE_BUILD_PARALLEL_LEVEL} "16")

