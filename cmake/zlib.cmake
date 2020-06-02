set(zlib_headers zconf.h zlib.h)
list(TRANSFORM zlib_headers PREPEND ${3RDPARTY_DIR}/zlib/)

set(zlib_sources
    adler32.c compress.c crc32.c
    deflate.c gzlib.c gzread.c gzwrite.c
    gzclose.c infback.c inffast.c inflate.c
    inftrees.c trees.c uncompr.c zutil.c)
list(TRANSFORM zlib_sources PREPEND ${3RDPARTY_DIR}/zlib/)

add_library(zlib STATIC ${zlib_headers}     ${zlib_sources})
set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
target_compile_options(zlib PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4267>)
