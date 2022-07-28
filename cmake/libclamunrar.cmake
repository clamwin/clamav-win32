# libclamunrar
file(GLOB libclamunrar_headers ${CLAMAV_DIR}/libclamunrar/*.hpp)

set(libclamunrar_sources
    archive.cpp arcread.cpp blake2s.cpp
    cmddata.cpp consio.cpp crc.cpp crypt.cpp
    dll.cpp encname.cpp errhnd.cpp extract.cpp
    filcreat.cpp file.cpp filefn.cpp filestr.cpp
    find.cpp getbits.cpp global.cpp hash.cpp
    headers.cpp match.cpp options.cpp
    pathfn.cpp qopen.cpp rar.cpp rarpch.cpp
    rarvm.cpp rawread.cpp rdwrfn.cpp rijndael.cpp
    scantree.cpp secpassword.cpp sha1.cpp
    sha256.cpp smallfn.cpp strfn.cpp strlist.cpp
    threadpool.cpp timefn.cpp ui.cpp unicode.cpp
    unpack.cpp volume.cpp)
list(TRANSFORM libclamunrar_sources PREPEND ${CLAMAV_DIR}/libclamunrar/)

set(libclamunrar_win32_sources
    ${CLAMWIN_DIR}/src/unrar/extinfo.cpp
    ${CLAMWIN_DIR}/src/unrar/isnt.cpp
    ${CLAMWIN_DIR}/src/unrar/system.cpp
    ${CLAMWIN_DIR}/resources/libclamunrar.rc
)

source_group("Win32 Files" FILES ${libclamunrar_win32_sources})

add_library(libclamunrar SHARED
    ${libclamunrar_headers}
    ${libclamunrar_sources}
    ${libclamunrar_win32_sources}
    ${CLAMWIN_DIR}/libclamunrar.def
)

set_target_properties(libclamunrar PROPERTIES DEFINE_SYMBOL "" PREFIX "" OUTPUT_NAME libclamunrar)
target_include_directories(libclamunrar PRIVATE ${CLAMAV_DIR}/ ${CLAMWIN_DIR}/resources)
target_compile_definitions(libclamunrar PRIVATE HAVE_CONFIG_H RARDLL WARN_DLOPEN_FAIL _FILE_OFFSET_BITS=64)

if((MINGW) AND (CLAMAV_ARCH STREQUAL "x86"))
    find_library(UNICOWS_LIBRARY
        NAMES libunicows.a unicows libunicows
        HINTS ${3RDPARTY_DIR}/libunicows
        NO_DEFAULT_PATH)
    target_link_libraries(libclamunrar PRIVATE ${UNICOWS_LIBRARY})
endif()

# libclamunrar_iface
add_library(libclamunrar_iface SHARED
    ${CLAMAV_DIR}/libclamunrar_iface/unrar_iface.cpp
    ${CLAMWIN_DIR}/resources/libclamunrar_iface.rc
    ${CLAMWIN_DIR}/libclamunrar_iface.def
)

set_target_properties(libclamunrar_iface PROPERTIES DEFINE_SYMBOL "" PREFIX "" OUTPUT_NAME libclamunrar_iface)
target_compile_definitions(libclamunrar_iface PRIVATE HAVE_CONFIG_H RARDLL WARN_DLOPEN_FAIL _FILE_OFFSET_BITS=64)
target_include_directories(libclamunrar_iface PRIVATE ${CLAMWIN_DIR}/include ${CLAMAV_DIR} ${CLAMAV_DIR}/libclamunrar ${CLAMWIN_DIR}/resources)
target_link_libraries(libclamunrar_iface PRIVATE libclamunrar)

list(APPEND CLAMAV_INSTALL_TARGETS libclamunrar libclamunrar_iface)
