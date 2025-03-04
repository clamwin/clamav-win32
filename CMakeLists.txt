cmake_minimum_required (VERSION 3.14)
project(clamav-win32 VERSION "1.4.2")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CLAMAV_ARCH x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set (CLAMAV_ARCH x86)
else()
    message(FATAL_ERROR "WTF")
endif()

set(CLAMWIN_DIR ${CMAKE_SOURCE_DIR})
set(CLAMAV_DIR ${CLAMWIN_DIR}/clamav)
set(3RDPARTY_DIR ${CLAMWIN_DIR}/3rdparty)

option(ENABLE_LLVM "Enable JIT using LLVM Engine" OFF)
option(WINXP "Enable support for WINXP" OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Do not build shared libraries" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)

include(${CLAMAV_DIR}/cmake/Version.cmake)

set(PACKAGE_NAME      "${PROJECT_NAME}")
set(PACKAGE_VERSION   "${PROJECT_VERSION}")
set(PACKAGE_STRING    "${PROJECT_NAME} ${PROJECT_VERSION}${VERSION_SUFFIX}")
set(PACKAGE_BUGREPORT "https://github.com/clamwin/clamav-win32")
set(PACKAGE_URL       "https://oss.netfarm.it/clamav/")
HexVersion(PACKAGE_VERSION_NUM ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})

# libtool library versioning rules: http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
set(LIBCLAMAV_CURRENT  12)
set(LIBCLAMAV_REVISION 3)
set(LIBCLAMAV_AGE      0)

math(EXPR LIBCLAMAV_SOVERSION "${LIBCLAMAV_CURRENT} - ${LIBCLAMAV_AGE}")
set(LIBCLAMAV_VERSION "${LIBCLAMAV_SOVERSION}.${LIBCLAMAV_AGE}.${LIBCLAMAV_REVISION}")
HexVersion(LIBCLAMAV_VERSION_NUM ${LIBCLAMAV_CURRENT} ${LIBCLAMAV_REVISION} ${LIBCLAMAV_AGE})

set(LIBFRESHCLAM_CURRENT  3)
set(LIBFRESHCLAM_REVISION 2)
set(LIBFRESHCLAM_AGE      0)

math(EXPR LIBFRESHCLAM_SOVERSION "${LIBFRESHCLAM_CURRENT} - ${LIBFRESHCLAM_AGE}")
set(LIBFRESHCLAM_VERSION "${LIBFRESHCLAM_SOVERSION}.${LIBFRESHCLAM_AGE}.${LIBFRESHCLAM_REVISION}")
HexVersion(LIBFRESHCLAM_VERSION_NUM ${LIBFRESHCLAM_CURRENT} ${LIBFRESHCLAM_REVISION} ${LIBFRESHCLAM_AGE})

add_custom_command(
        OUTPUT libclamav.dev
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/genversion.py
        ${CMAKE_CURRENT_BINARY_DIR}/viewer_version.hpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/genversion.py ${GIT_HEAD} ${BUILD_GRADLE}
)

set(CLAMWIN_LIBRARIES)

set(CLAMWIN_INCLUDES
    ${CMAKE_BINARY_DIR}
    ${CLAMWIN_DIR}/include
    ${CLAMAV_DIR}/common
    ${CLAMAV_DIR}/win32/compat
    ${CLAMAV_DIR}/libclamav
    ${CLAMAV_DIR}/libclamav_rust
    ${CLAMAV_DIR}/libclamunrar_iface
    ${CLAMAV_DIR}/libclammspack/mspack
)

set(CLAMWIN_DEFINES
    CLAMWIN
    HAVE_CONFIG_H
    WIN32_LEAN_AND_MEAN
    PCRE2_STATIC
    LIBXML_STATIC
)

configure_file(libclamav.def ${CMAKE_BINARY_DIR}/libclamav.def @ONLY)

if(TRUE)
    set(WITH_WINPTHREADS TRUE)
    set(WINPTHREADS_DIR ${3RDPARTY_DIR}/mingw-w64/mingw-w64-libraries/winpthreads)
    list(APPEND CLAMWIN_INCLUDES ${WINPTHREADS_DIR}/include)
    file(READ winpthreads.def WINPTHREADS_CONTENT)
    file(APPEND ${CMAKE_BINARY_DIR}/libclamav.def "\n${WINPTHREADS_CONTENT}")
endif()

if(WINXP)
    if(MSVC)
        message(FATAL_ERROR "WinXP Compatibility is MinGW Only")
    endif()
    list(APPEND CLAMWIN_DEFINES _WIN32_WINNT=_WIN32_WINNT_WINXP)
else()
    list(APPEND CLAMWIN_DEFINES _WIN32_WINNT=_WIN32_WINNT_VISTA)
endif()

configure_file(${CLAMAV_DIR}/clamav-version.h.in ${CMAKE_BINARY_DIR}/clamav-version.h)

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

if(ENABLE_LLVM)
    message(STATUS "LLVM is ENABLED")
    include(cmake/llvm.cmake)
else()
    message(STATUS "LLVM is DISABLED")
endif()

include(cmake/openssl.cmake)
include(cmake/zlib.cmake)
include(cmake/bzip2.cmake)
include(cmake/pcre2.cmake)
include(cmake/json-c.cmake)
include(cmake/libxml2.cmake)
include(cmake/libclammspack.cmake)
include(cmake/libclamav.cmake)
include(cmake/libclamav_common.cmake)
include(cmake/clambc.cmake)
include(cmake/clamd.cmake)
include(cmake/clamdscan.cmake)
include(cmake/clamscan.cmake)
include(cmake/freshclam.cmake)
include(cmake/sigtool.cmake)
include(cmake/libclamunrar.cmake)

include(cmake/pdcurses.cmake)
include(cmake/clamdtop.cmake)

include(${CLAMAV_DIR}/cmake/FindRust.cmake)
if(MINGW)
    set(RUST_NATIVE_STATIC_LIBS "-lntdll -lws2_32 -lpsapi")
endif()
add_subdirectory(${CLAMAV_DIR}/libclamav_rust)

if(WINXP)
    include(cmake/winxp.cmake)
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
