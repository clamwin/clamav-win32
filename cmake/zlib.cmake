set(zlib_srcs
    adler32.c compress.c crc32.c
    deflate.c gzlib.c gzread.c gzwrite.c
    gzclose.c infback.c inffast.c inflate.c
    inftrees.c trees.c uncompr.c zutil.c)
list(TRANSFORM zlib_srcs PREPEND ${3RDPARTY}/zlib/)

add_library(zlib STATIC ${zlib_srcs})
set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
