cmake_minimum_required (VERSION 3.6)
project(clamav-win32)

set(CLAMAV ${CMAKE_SOURCE_DIR}/clamav)
set(3RDPARTY ${CMAKE_SOURCE_DIR}/3rdparty)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/resources)
include_directories(${3RDPARTY} ${3RDPARTY}/zlib ${3RDPARTY}/pthreads ${3RDPARTY}/pcre2/src ${3RDPARTY}/openssl/include)
include_directories(${CLAMAV} ${CLAMAV}/shared)

add_compile_definitions(_WIN32 WIN32_LEAN_AND_MEAN)
add_compile_definitions(CLAMWIN)

if(MINGW)
    link_libraries("-static")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-stdcall-fixup ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-ms-bitfields")
elseif(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/wd4996 /wd4005)
endif()

include(cmake/gnulib.cmake)
include(cmake/zlib.cmake)
include(cmake/bzip2.cmake)
include(cmake/pcre2.cmake)
include(cmake/json-c.cmake)
include(cmake/libxml2.cmake)
include(cmake/libclammspack.cmake)

include(cmake/libclamav.cmake)
include(cmake/clambc.cmake)
include(cmake/clamd.cmake)
include(cmake/clamdscan.cmake)
include(cmake/clamscan.cmake)
include(cmake/freshclam.cmake)
include(cmake/sigtool.cmake)
include(cmake/libclamunrar.cmake)

include(cmake/llvm.cmake)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT clamscan)