option(USE_ZLIB_NG_ON_X86 "Use zlib-ng for x86 builds" OFF)

if(CLAMAV_ARCH STREQUAL "x86" AND NOT USE_ZLIB_NG_ON_X86)
    set(ZLIB_DIR ${3RDPARTY_DIR}/zlib)
    set(zlib_headers ${ZLIB_DIR}/zconf.h ${ZLIB_DIR}/zlib.h)
    list(APPEND CLAMWIN_INCLUDES ${ZLIB_DIR})

    set(zlib_sources
        adler32.c compress.c crc32.c
        deflate.c gzlib.c gzread.c gzwrite.c
        gzclose.c infback.c inffast.c inflate.c
        inftrees.c trees.c uncompr.c zutil.c)
    list(TRANSFORM zlib_sources PREPEND ${ZLIB_DIR}/)

    message(STATUS "==== Adding subproject zlib ====")
    add_library(zlibstatic STATIC ${zlib_headers} ${zlib_sources})
    target_compile_options(zlibstatic PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4267>)
    install(FILES ${ZLIB_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME zlib.txt)
else()
    set(ZLIB_DIR ${3RDPARTY_DIR}/zlib-ng)
    set(ZLIB_COMPAT ON)
    set(BUILD_TESTING OFF)
    set(ZLIBNG_ENABLE_TESTS OFF)
    set(WITH_GTEST OFF)

    if(MSVC)
        set(HAVE_UNISTD_H 0)
    endif()

    message(STATUS "==== Adding subproject zlib-ng ====")
    add_subdirectory(${ZLIB_DIR} EXCLUDE_FROM_ALL)
    list(APPEND CLAMWIN_INCLUDES ${zlib_BINARY_DIR})
    install(FILES ${ZLIB_DIR}/LICENSE.md DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME zlib-ng.md)
endif()

set(ZLIB_FOUND TRUE)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_INCLUDE_DIR ${ZLIB_DIR})

list(APPEND CLAMWIN_LIBRARIES zlibstatic)
