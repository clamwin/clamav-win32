if(MINGW)
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY}/openssl/lib/mingw/${CLAMAV_ARCH})
elseif(MSVC)
    set(OPENSSL_LIBRARY_PATH ${3RDPARTY}/openssl/lib/msvc/${CLAMAV_ARCH})
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

find_library(OPENSSL_SSL_LIBRARY
    NAMES ssl libssl
    HINTS ${OPENSSL_LIBRARY_PATH}
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES crypto libcrypto
    HINTS ${OPENSSL_LIBRARY_PATH}
)

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

file(GLOB libclamav_win32 ${CMAKE_SOURCE_DIR}/src/dllmain/*.c)
if(MINGW)
    list(APPEND libclamav_win32 ${CMAKE_SOURCE_DIR}/src/dllmain/pthread-mingw.c)
else()
    list(APPEND libclamav_win32 ${3RDPARTY}/pthreads/pthread.c)
endif()

file(GLOB_RECURSE libclamav_win32_headers ${CMAKE_SOURCE_DIR}/include/*.h)

source_group("Header Files" FILES ${libclamav_win32_headers})
source_group("Source Files" FILES ${libclamav_srcs})
source_group("Win32 Files" FILES ${libclamav_win32})

add_library(libclamav SHARED
    ${libclamav_win32_headers}
    ${libclamav_srcs}
    ${libclamav_win32}
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
    libxml2
    clammspack
    gnulib
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ws2_32)

set_target_properties(libclamav PROPERTIES
    DEFINE_SYMBOL LIBCLAMAV_EXPORTS
    PREFIX ""
    OUTPUT_NAME libclamav)

if(MSVC)
    set_target_properties(libclamav PROPERTIES PUBLIC_HEADER ${CLAMAV}/libclamav/clamav.h)
endif()

list(APPEND CLAMAV_INSTALL_TARGETS libclamav)

install(FILES ${3RDPARTY}/openssl/LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.openssl)
install(FILES ${3RDPARTY}/pthreads/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.pthreads-win32)
install(FILES ${3RDPARTY}/gnulib/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.gnulib)
install(FILES ${3RDPARTY}/libunicows/license.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME COPYING.libunicows)
