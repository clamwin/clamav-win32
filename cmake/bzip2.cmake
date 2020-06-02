set(bzip2_headers ${3RDPARTY_DIR}/bzip2/bzlib.h)

set(bzip2_sources blocksort.c bzlib.c compress.c crctable.c decompress.c huffman.c randtable.c)
list(TRANSFORM bzip2_sources PREPEND ${3RDPARTY_DIR}/bzip2/)

add_library(bzip2 STATIC ${bzip2_headers} ${bzip2_sources})
set_target_properties(bzip2 PROPERTIES OUTPUT_NAME bz2)
target_compile_options(bzip2 PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4244 /wd4267>)
