set(libclamunrar_srcs
    archive.cpp arcread.cpp blake2s.cpp
    cmddata.cpp consio.cpp crc.cpp crypt.cpp
    dll.cpp encname.cpp errhnd.cpp extract.cpp
    filcreat.cpp file.cpp filefn.cpp filestr.cpp
    find.cpp getbits.cpp global.cpp hash.cpp
    headers.cpp isnt.cpp match.cpp options.cpp
    pathfn.cpp qopen.cpp rar.cpp rarpch.cpp
    rarvm.cpp rawread.cpp rdwrfn.cpp rijndael.cpp
    scantree.cpp secpassword.cpp sha1.cpp
    sha256.cpp smallfn.cpp strfn.cpp strlist.cpp
    threadpool.cpp timefn.cpp ui.cpp unicode.cpp
    unpack.cpp volume.cpp)

list(TRANSFORM libclamunrar_srcs PREPEND ${CLAMAV}/libclamunrar/)
list(APPEND libclamunrar_srcs
    ${CMAKE_SOURCE_DIR}/src/unrar/extinfo.cpp
    ${CMAKE_SOURCE_DIR}/src/unrar/system.cpp
    ${CMAKE_SOURCE_DIR}/resources/libclamunrar.rc
    ${CMAKE_SOURCE_DIR}/libclamunrar.def)

add_library(libclamunrar SHARED ${libclamunrar_srcs})
target_compile_definitions(libclamunrar PRIVATE HAVE_CONFIG_H RARDLL)
set_target_properties(libclamunrar PROPERTIES
    DEFINE_SYMBOL ""
    PREFIX ""
    OUTPUT_NAME libclamunrar)

if(MINGW)
    find_library(UNICOWS_LIBRARY
        NAMES libunicows.a unicows libunicows
        HINTS ${3RDPARTY}/libunicows
        NO_DEFAULT_PATH)
    target_link_libraries(libclamunrar PRIVATE ${UNICOWS_LIBRARY})
endif()

add_library(libclamunrar_iface SHARED
    ${CLAMAV}/libclamunrar_iface/unrar_iface.cpp
    ${CMAKE_SOURCE_DIR}/resources/libclamunrar_iface.rc
    ${CMAKE_SOURCE_DIR}/libclamunrar_iface.def)

target_compile_definitions(libclamunrar_iface PRIVATE HAVE_CONFIG_H RARDLL)
target_link_libraries(libclamunrar_iface PRIVATE libclamunrar)
set_target_properties(libclamunrar_iface PROPERTIES
    DEFINE_SYMBOL ""
    PREFIX ""
    OUTPUT_NAME libclamunrar_iface)

list(APPEND CLAMAV_INSTALL_TARGETS libclamunrar libclamunrar_iface)
