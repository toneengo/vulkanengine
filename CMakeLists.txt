cmake_minimum_required(VERSION 3.12)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS OFF)

project("vulkanengine")

add_executable(vulkanengine)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "vulkanengine")

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vulkanengine PRIVATE glfw)
find_package(Vulkan REQUIRED)
target_link_libraries(vulkanengine PRIVATE Vulkan::Vulkan)
find_package(glslang REQUIRED)
target_link_libraries(vulkanengine PRIVATE glslang::glslang)
target_link_libraries(vulkanengine PRIVATE glslang::glslang-default-resource-limits)
target_link_libraries(vulkanengine PRIVATE glslang::glslang)
target_link_libraries(vulkanengine PRIVATE glslang::glslang-default-resource-limits)

find_package(assimp REQUIRED)
target_link_libraries(vulkanengine PRIVATE assimp::assimp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap)
target_link_libraries(vulkanengine PRIVATE vk-bootstrap::vk-bootstrap)

target_include_directories(vulkanengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/")
target_include_directories(vulkanengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
set(env{CMAKE_BUILD_PARALLEL_LEVEL} "16")

add_custom_target(assets 
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets)

add_dependencies(vulkanengine assets)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(STATUS "Configuring Sector33 in Debug with CMake")
    add_compile_definitions(DBG)
else()
    add_compile_options(-O3)
    message(STATUS "Configuring Sector33 in Release with CMake")
endif()

target_include_directories(vulkanengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include")
target_include_directories(vulkanengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")
target_include_directories(vulkanengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")
target_sources(vulkanengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp")
