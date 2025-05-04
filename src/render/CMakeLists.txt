add_subdirectory(vk)
target_sources(spockengine PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/render_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/input.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/input.cpp)
