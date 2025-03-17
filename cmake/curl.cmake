set(CURL_DIR ${3RDPARTY_DIR}/curl)

set(HTTP_ONLY ON)

set(CURL_DISABLE_KERBEROS_AUTH ON)
set(CURL_DISABLE_AWS ON)
set(CURL_DISABLE_WEBSOCKETS ON)

set(USE_NGHTTP2 OFF)
set(USE_NGTCP2 OFF)
set(CURL_USE_OPENSSL OFF)
set(CURL_USE_LIBPSL OFF)
set(CURL_USE_LIBSSH2 OFF)

set(ENABLE_CURL_MANUAL OFF)
set(CURL_DISABLE_INSTALL ON)
set(CURL_ENABLE_EXPORT_TARGET OFF)

set(BUILD_STATIC_CURL ON)
set(BUILD_CURL_EXE OFF)
set(BUILD_LIBCURL_DOCS OFF)
set(BUILD_MISC_DOCS OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_TESTING OFF)

set(CURL_ZLIB OFF)
set(CURL_BROTLI OFF)
set(CURL_ZSTD OFF)

set(_ssl_enabled ON)
set(USE_OPENSSL ON)
set(HAVE_SSL_SET0_WBIO 1)
set(HAVE_OPENSSL_SRP 0)

if(ENABLE_LEGACY STREQUAL "win9x")
    set(USE_WIN32_IDN OFF)
    set(ENABLE_UNICODE OFF)
    set(ENABLE_IPV6 OFF)
    set(ENABLE_THREADED_RESOLVER OFF)
else()
    set(USE_WIN32_IDN ON)
    set(ENABLE_UNICODE ON)
endif()

if(MSVC)
    set(HAVE_SIZEOF_SSIZE_T FALSE)
endif()

if(NOT ENABLE_LEGACY STREQUAL "OFF")
    set(CURL_TARGET_WINDOWS_VERSION "0x0501" CACHE STRING "Minimum target Windows version as hex string")
endif()

add_subdirectory(${CURL_DIR} EXCLUDE_FROM_ALL)
target_include_directories(libcurl_object PRIVATE ${OPENSSL_INCLUDE_DIR})

if(ENABLE_LEGACY STREQUAL "win9x")
    set_source_files_properties(
        ${CURL_DIR}/lib/version_win32.c
        DIRECTORY ${CURL_DIR}/lib
        PROPERTIES COMPILE_FLAGS "-D_WIN32_WINNT=0x0400"
    )
    set_source_files_properties(
        ${CURL_DIR}/lib/asyn-thread.c
        ${CURL_DIR}/lib/curl_addrinfo.c
        ${CURL_DIR}/lib/hostasyn.c
        ${CURL_DIR}/lib/hostip.c
        ${CURL_DIR}/lib/hostip4.c
        DIRECTORY ${CURL_DIR}/lib
        PROPERTIES COMPILE_FLAGS "-include wspiapi.h")
endif()

install(FILES ${CURL_DIR}/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME curl.txt)
