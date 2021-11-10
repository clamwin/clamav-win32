cmake_minimum_required (VERSION 3.6)
project(clamav-win32)

set(CLAMWIN_DIR ${CMAKE_SOURCE_DIR})
set(CLAMAV_DIR ${CLAMWIN_DIR}/clamav)
set(3RDPARTY_DIR ${CLAMWIN_DIR}/3rdparty)

option(DISABLE_LLVM "Disable LLVM Engine" OFF)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
add_compile_options($<$<C_COMPILER_ID:MSVC>:/MP>)

set(CLAMWIN_INCLUDES
    ${CLAMWIN_DIR}/resources
    ${CLAMWIN_DIR}/include
    ${CLAMWIN_DIR}/include/json-c
    ${CLAMWIN_DIR}/include/libxml2
    ${CLAMWIN_DIR}/include/pcre2
    ${CLAMAV_DIR}
    ${CLAMAV_DIR}/shared
    ${CLAMAV_DIR}/libclamav
    ${CLAMAV_DIR}/libclamunrar_iface
    ${CLAMAV_DIR}/libclammspack/mspack
    ${3RDPARTY_DIR}
    ${3RDPARTY_DIR}/zlib
    ${3RDPARTY_DIR}/bzip2
    ${3RDPARTY_DIR}/json-c
    ${3RDPARTY_DIR}/libxml2/include
    ${3RDPARTY_DIR}/openssl/include
    ${3RDPARTY_DIR}/pcre2/src
    ${3RDPARTY_DIR}/pthreads
)

set(CLAMWIN_DEFINES
    CLAMWIN
    HAVE_CONFIG_H
    WIN32_LEAN_AND_MEAN
    PCRE2_STATIC
    PTW32_STATIC_LIB
    LIBXML_STATIC
    NOBZ2PREFIX
)

if(MINGW)
    link_libraries("-static")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-stdcall-fixup ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-ms-bitfields")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
elseif(MSVC)
    set(CMAKE_DEBUG_POSTFIX d)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/wd4996 /wd4005)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CLAMAV_ARCH x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set (CLAMAV_ARCH x86)
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

include(cmake/pdcurses.cmake)
include(cmake/clamdtop.cmake)

if(DISABLE_LLVM)
    message(STATUS "LLVM is DISABLED")
else()
    message(STATUS "LLVM is ENABLED")
    include(cmake/llvm.cmake)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT clamscan)

install(TARGETS ${CLAMAV_INSTALL_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}
)

set(DIST_FILES README.md clamd.conf freshclam.conf clamav.reg)

if(NOT MINGW)
    list(APPEND DIST_FILES include/clamav-types.h)
    install(TARGETS libclamav
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}
        PUBLIC_HEADER EXCLUDE_FROM_ALL)
endif()

file(GLOB COPYRIGHT_FILES ${CLAMAV}/COPYING*)
install(FILES ${COPYRIGHT_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright)

install(FILES ${DIST_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX})
