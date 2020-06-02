set(gnulib_headers ${3RDPARTY_DIR}/gnulib/config.h)

file(GLOB gnulib_sources ${3RDPARTY_DIR}/gnulib/*.c)
list(REMOVE_ITEM gnulib_sources ${3RDPARTY_DIR}/gnulib/strtol.c)

add_library(gnulib STATIC ${gnulib_headers} ${gnulib_sources})
target_include_directories(gnulib PRIVATE ${3RDPARTY_DIR}/gnulib ${CLAMWIN_DIR}/include)
target_compile_definitions(gnulib PRIVATE HAVE_CONFIG_H)
set_target_properties(gnulib PROPERTIES PREFIX "")

if(MINGW)
    target_compile_definitions(gnulib PRIVATE XSIZE_INLINE=static)
endif()
