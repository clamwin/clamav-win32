set(BZIP2_DIR ${3RDPARTY_DIR}/bzip2)
set(bzip2_headers ${BZIP2_DIR}/bzlib.h)

set(bzip2_sources blocksort.c bzlib.c compress.c crctable.c decompress.c huffman.c randtable.c)
list(TRANSFORM bzip2_sources PREPEND ${BZIP2_DIR}/)

add_library(bzip2 STATIC ${bzip2_headers} ${bzip2_sources})
set_target_properties(bzip2 PROPERTIES OUTPUT_NAME bz2)
target_compile_options(bzip2 PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4244 /wd4267>)

list(APPEND CLAMWIN_INCLUDES ${BZIP2_DIR})
list(APPEND CLAMWIN_LIBRARIES bzip2)

set(BZIP2_FOUND TRUE)
set(BZIP2_LIBRARY bzip2)
set(BZIP2_LIBRARIES bzip2)
set(BZIP2_INCLUDE_DIR ${BZIP2_DIR})

install(FILES ${BZIP2_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME bzip2.txt)
