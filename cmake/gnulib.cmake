file(GLOB gnulib_srcs ${3RDPARTY}/gnulib/*.c)
add_library(gnulib STATIC ${gnulib_srcs})
target_include_directories(gnulib PRIVATE ${3RDPARTY}/gnulib)
target_compile_definitions(gnulib PRIVATE HAVE_CONFIG_H)

if(MINGW)
    target_compile_definitions(gnulib PRIVATE XSIZE_INLINE=static)
endif()
