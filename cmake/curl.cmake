file(GLOB curl_sources ${3RDPARTY_DIR}/curl/lib/*.c ${3RDPARTY_DIR}/curl/lib/*/*.c)

add_library(curl STATIC ${curl_sources})

target_include_directories(curl PRIVATE
    ${3RDPARTY_DIR}/curl/include
    ${3RDPARTY_DIR}/curl/lib
    ${3RDPARTY_DIR}/pthreads
    ${3RDPARTY_DIR}/openssl/include
)

target_compile_definitions(curl PRIVATE
    BUILDING_LIBCURL
    CURL_STATICLIB
    HTTP_ONLY
    USE_OPENSSL
    HAVE_PTHREAD_H
    USE_THREADS_POSIX
)

if((MINGW) AND (CLAMAV_ARCH STREQUAL "x86"))
    target_compile_definitions(curl PRIVATE _WIN32_WINNT=0x400)
endif()
