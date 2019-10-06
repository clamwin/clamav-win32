set(bzip2_srcs blocksort.c bzlib.c compress.c crctable.c decompress.c huffman.c randtable.c)
list(TRANSFORM bzip2_srcs PREPEND ${3RDPARTY}/bzip2/)
add_library(bzip2 STATIC ${bzip2_srcs})
set_target_properties(bzip2 PROPERTIES OUTPUT_NAME bz2)
