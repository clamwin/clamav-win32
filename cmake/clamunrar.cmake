set(clamunrar_srcs
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

list(TRANSFORM clamunrar_srcs PREPEND ${CLAMAV}/libclamunrar/)
list(APPEND clamunrar_srcs
    ${CMAKE_SOURCE_DIR}/src/unrar/extinfo.cpp
    ${CMAKE_SOURCE_DIR}/src/unrar/system.cpp
    ${CMAKE_SOURCE_DIR}/resources/libclamunrar.rc
    ${CMAKE_SOURCE_DIR}/libclamunrar.def)

add_library(clamunrar SHARED ${clamunrar_srcs})
target_compile_definitions(clamunrar PRIVATE HAVE_CONFIG_H RARDLL)
set_target_properties(clamunrar PROPERTIES DEFINE_SYMBOL "")

if(MINGW)
    find_library(UNICOWS_LIBRARY
        NAMES unicows libunicows
        HINTS "${3RDPARTY}/libunicows")
    target_link_libraries(clamunrar PRIVATE ${UNICOWS_LIBRARY})
endif()

add_library(clamunrar_iface SHARED
    ${CLAMAV}/libclamunrar_iface/unrar_iface.cpp
    ${CMAKE_SOURCE_DIR}/resources/libclamunrar_iface.rc
    ${CMAKE_SOURCE_DIR}/libclamunrar_iface.def)

target_compile_definitions(clamunrar_iface PRIVATE HAVE_CONFIG_H RARDLL)
set_target_properties(clamunrar_iface PROPERTIES DEFINE_SYMBOL "")
target_link_libraries(clamunrar_iface PRIVATE clamunrar)
