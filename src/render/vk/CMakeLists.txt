file(GLOB_RECURSE src_f CONFIGURE_DEPENDS
     "*.hpp"
     "*.cpp"
)

target_sources(spock PRIVATE ${src_f})
