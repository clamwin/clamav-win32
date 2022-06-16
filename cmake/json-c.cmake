file(GLOB_RECURSE json-c_headers ${CLAMWIN_DIR}/include/json-c/*.h)
file(GLOB json-c_sources ${3RDPARTY_DIR}/json-c/*.c)

add_library(json-c STATIC ${json-c_headers} ${json-c_sources})
target_include_directories(json-c PRIVATE ${CLAMWIN_DIR}/include/json-c ${CLAMWIN_DIR}/include ${3RDPARTY_DIR}/json-c)
target_compile_definitions(json-c PRIVATE HAVE_CONFIG_H)
target_compile_options(json-c PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4244 /wd4267>)

install(FILES ${3RDPARTY_DIR}/json-c/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.json-c)
