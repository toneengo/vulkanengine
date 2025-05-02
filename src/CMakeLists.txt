file(GLOB_RECURSE src_f CONFIGURE_DEPENDS "*.hpp"
                                          "*.cpp"
                                          "*.c")

target_sources(vulkanengine PRIVATE ${src_f})
