file(GLOB libclamav_srcs
    ${CLAMAV}/libclamav/*.c
    ${CLAMAV}/libclamav/7z/*.c
    ${CLAMAV}/libclamav/lzw/*.c
    ${CLAMAV}/libclamav/nsis/*.c
    ${CLAMAV}/libclamav/regex/*.c
    ${CLAMAV}/libclamav/tomsfastmath/*/*.c
    ${CLAMAV}/libclamav/jsparse/js-norm.c)

list(REMOVE_ITEM libclamav_srcs
    ${CLAMAV}/libclamav/others.c
    ${CLAMAV}/libclamav/regex/engine.c
    ${CLAMAV}/libclamav/bytecode_nojit.c
    ${CLAMAV}/libclamav/tomsfastmath/misc/fp_ident.c)

file(GLOB libclamav_dllmain ${CMAKE_SOURCE_DIR}/src/dllmain/*.c)
list(APPEND libclamav_srcs ${libclamav_dllmain})

if(MINGW)
    list(APPEND libclamav_srcs ${CMAKE_SOURCE_DIR}/src/dllmain/pthread-mingw.c)
endif()

link_directories(${3RDPARTY}/openssl/lib/mingw32)
add_library(clamav SHARED
    ${libclamav_srcs}
    ${CMAKE_SOURCE_DIR}/resources/libclamav.rc
    ${CMAKE_SOURCE_DIR}/libclamav.def)

target_link_libraries(clamav PRIVATE
    zlib
    bzip2
    pcre2
    json-c
    xml2
    clammspack
    gnulib
    ssl
    crypto
    ws2_32)

set_target_properties(clamav PROPERTIES DEFINE_SYMBOL LIBCLAMAV_EXPORTS)

target_include_directories(clamav PRIVATE
    ${3RDPARTY}/pthreads
    ${3RDPARTY}/json-c
    ${3RDPARTY}/libxml2/include
    ${CLAMAV}/libclamav
    ${CLAMAV}/libclammspack/mspack)

target_compile_definitions(clamav PRIVATE HAVE_CONFIG_H PCRE2_STATIC PTW32_STATIC_LIB LIBXML_STATIC BZ_IMPORT)
