file(GLOB curl_sources ${3RDPARTY_DIR}/curl/lib/*.c ${3RDPARTY_DIR}/curl/lib/*/*.c)

add_library(curl STATIC ${curl_sources})

target_include_directories(curl PRIVATE
    ${3RDPARTY_DIR}/curl/include
    ${3RDPARTY_DIR}/curl/lib
    ${3RDPARTY_DIR}/openssl/include
    ${3RDPARTY_DIR}/zlib
)

target_compile_definitions(curl PRIVATE
    BUILDING_LIBCURL
    CURL_STATICLIB

    HTTP_ONLY
    USE_OPENSSL
    USE_THREADS_WIN32
    ENABLE_IPV6

    HAVE_GETADDRINFO=1
    HAVE_GETADDRINFO_THREADSAFE=1
    HAVE_IOCTLSOCKET=1
    HAVE_IOCTLSOCKET_FIONBIO=1
    HAVE_RECV=1
    HAVE_SEND=1

    HAVE_WINDOWS_H=1
    HAVE_WS2TCPIP_H=1
    HAVE_WINSOCK2_H=1
    HAVE_GETHOSTNAME=1
    HAVE_LIBZ=1
    HAVE_FCNTL_H=1
    HAVE_IO_H=1
    HAVE_SYS_STAT_H=1
    HAVE_SYS_TYPES_H=1
    HAVE_SYS_UTIME_H=1
    HAVE_SOCKET=1
    HAVE_SELECT=1
    HAVE_STRDUP=1
    HAVE_STRICMP=1
    HAVE_STRCMPI=1
    HAVE_CLOSESOCKET=1
    HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID=1
    HAVE_SIGNAL=1
    STDC_HEADERS=1
)

target_link_libraries(curl PRIVATE zlib)

if((MINGW) AND (CLAMAV_ARCH STREQUAL "x86"))
    target_compile_definitions(curl PRIVATE _WIN32_WINNT=0x400)
else()
    target_link_libraries(curl PRIVATE bcrypt)
endif()
