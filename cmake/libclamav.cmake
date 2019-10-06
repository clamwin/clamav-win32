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
else()
    list(APPEND libclamav_srcs ${3RDPARTY}/pthreads/pthread.c)
endif()

if(MINGW)
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY}/openssl/lib/mingw32)
elseif(CMAKE_CL_64)
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY}/openssl/lib/x64)
else()
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY}/openssl/lib/Win32)
endif()

find_library(OPENSSL_SSL_LIBRARY
    NAMES ssl libssl
    HINTS ${OPENSSL_LIBRARY_PATH}
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES crypto libcrypto
    HINTS ${OPENSSL_LIBRARY_PATH}
)

add_library(libclamav SHARED
    ${libclamav_srcs}
    ${CMAKE_SOURCE_DIR}/resources/libclamav.rc
    ${CMAKE_SOURCE_DIR}/libclamav.def)

target_include_directories(libclamav PRIVATE
    ${3RDPARTY}/bzip2
    ${3RDPARTY}/json-c
    ${3RDPARTY}/libxml2/include
    ${CLAMAV}/libclamav
    ${CLAMAV}/libclammspack/mspack)

target_compile_definitions(libclamav PRIVATE HAVE_CONFIG_H PCRE2_STATIC PTW32_STATIC_LIB LIBXML_STATIC NOBZ2PREFIX)

target_link_libraries(libclamav PRIVATE
    zlib
    bzip2
    pcre2
    json-c
    xml2
    clammspack
    gnulib
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ws2_32)

if(NOT MINGW)
target_link_libraries(libclamav PRIVATE legacy_stdio_definitions)
endif()

set_target_properties(libclamav PROPERTIES
    DEFINE_SYMBOL LIBCLAMAV_EXPORTS
    PREFIX ""
    OUTPUT_NAME libclamav)
