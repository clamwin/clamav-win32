cmake_minimum_required (VERSION 3.6)
project(clamav-win32)

set(CLAMWIN_DIR ${CMAKE_SOURCE_DIR})
set(CLAMAV_DIR ${CLAMWIN_DIR}/clamav)
set(3RDPARTY_DIR ${CLAMWIN_DIR}/3rdparty)

option(DISABLE_LLVM "Disable LLVM Engine" OFF)
set(BUILD_PTHREADS TRUE)

if(MINGW)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(${GCC_VERSION} MATCHES "posix$")
        set(BUILD_PTHREADS FALSE)
    endif()
endif()

add_custom_command(
        OUTPUT libclamav.dev
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/genversion.py
        ${CMAKE_CURRENT_BINARY_DIR}/viewer_version.hpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/genversion.py ${GIT_HEAD} ${BUILD_GRADLE}
)

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
)

set(CLAMWIN_DEFINES
    CLAMWIN
    HAVE_CONFIG_H
    WIN32_LEAN_AND_MEAN
    PCRE2_STATIC
    LIBXML_STATIC
    NOBZ2PREFIX
)

configure_file(libclamav.def ${CMAKE_BINARY_DIR}/libclamav.def @ONLY)

if(BUILD_PTHREADS)
    list(APPEND CLAMWIN_INCLUDES ${3RDPARTY_DIR}/pthreads)
    list(APPEND CLAMWIN_DEFINES PTW32_STATIC_LIB)

    file(READ pthreads.def PTHREADS_CONTENT)
    file(APPEND ${CMAKE_BINARY_DIR}/libclamav.def "\n${PTHREADS_CONTENT}")
endif()

if(MINGW)
    link_libraries("-static")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-stdcall-fixup ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-ms-bitfields")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-register>)
elseif(MSVC)
    set(CMAKE_DEBUG_POSTFIX d)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:/MP$<SEMICOLON>/wd4996$<SEMICOLON>/wd4005>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/MP$<SEMICOLON>/wd4996$<SEMICOLON>/wd4005>)
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

file(GLOB COPYRIGHT_FILES ${CLAMAV}/COPYING*)
install(FILES ${COPYRIGHT_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright)

install(FILES ${DIST_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX})
