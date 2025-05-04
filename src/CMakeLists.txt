add_subdirectory(render)

if(SPOCK_BUILD_ENGINE)
    target_sources(spockengine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
endif()
