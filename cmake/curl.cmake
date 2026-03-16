set(CURL_DIR ${3RDPARTY_DIR}/curl)

set(HTTP_ONLY ON)

set(CURL_DISABLE_KERBEROS_AUTH ON)
set(CURL_DISABLE_AWS ON)
set(CURL_DISABLE_WEBSOCKETS ON)

set(USE_NGHTTP2 OFF)
set(USE_NGTCP2 OFF)
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
set(HAVE_DES_ECB_ENCRYPT 0)

# must be OFF or libfreshclam will get openssl direct dependency, we export symbols in libclamav
set(CURL_USE_OPENSSL OFF)

set(USE_WIN32_IDN OFF)
set(ENABLE_IPV6 OFF)
set(ENABLE_THREADED_RESOLVER OFF)

set(CURL_TARGET_WINDOWS_VERSION "0x0600" CACHE STRING "Minimum target Windows version as hex string" FORCE)

if(MSVC)
    set(HAVE_SIZEOF_SSIZE_T FALSE)
endif()

message(STATUS "==== Adding subproject curl ====")
add_subdirectory(${CURL_DIR} EXCLUDE_FROM_ALL)
target_include_directories(libcurl_object PRIVATE ${OPENSSL_INCLUDE_DIR})

set_source_files_properties(
    ${CURL_DIR}/lib/curlx/version_win32.c
    ${CURL_DIR}/lib/system_win32.c
    DIRECTORY ${CURL_DIR}/lib
    PROPERTIES COMPILE_DEFINITIONS "CURL_WINDOWS_UWP"
)
set_source_files_properties(
    ${CURL_DIR}/lib/asyn-thread.c
    ${CURL_DIR}/lib/curl_addrinfo.c
    ${CURL_DIR}/lib/hostasyn.c
    ${CURL_DIR}/lib/hostip.c
    ${CURL_DIR}/lib/hostip4.c
    DIRECTORY ${CURL_DIR}/lib
    PROPERTIES COMPILE_FLAGS "$<$<CXX_COMPILER_ID:GNU>:-include wspiapi.h>")

find_package(Perl REQUIRED)

set(CURL_CA_BUNDLE_FILE "${CURL_BINARY_DIR}/lib/curl-ca-bundle.crt")

add_custom_command(
    OUTPUT ${CURL_CA_BUNDLE_FILE}
    COMMENT "Generating a fresh curl-ca-bundle.crt" VERBATIM USES_TERMINAL
    COMMAND "${PERL_EXECUTABLE}" "${CURL_DIR}/scripts/mk-ca-bundle.pl" -b -l -u "${CURL_CA_BUNDLE_FILE}"
    WORKING_DIRECTORY ${CURL_DIR}
    DEPENDS "${CURL_DIR}/scripts/mk-ca-bundle.pl"
)

add_custom_target(generate-ca-bundle ALL DEPENDS ${CURL_CA_BUNDLE_FILE})
install(FILES ${CURL_CA_BUNDLE_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX})

install(FILES ${CURL_DIR}/COPYING DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME curl.txt)
