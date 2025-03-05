file(GLOB clamscan_headers ${CLAMAV_DIR}/clamscan/*.h)
file(GLOB clamscan_sources ${CLAMAV_DIR}/clamscan/*.c)

set(clamscan_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMAV_DIR}/common/scanmem.c
    ${CLAMAV_DIR}/common/exeScanner.c
)

source_group("Win32 Files" FILES ${clamscan_win32_sources})

add_executable(clamscan
    ${clamscan_headers}
    ${clamscan_sources}
    ${clamscan_win32_sources}
    ${CLAMWIN_DIR}/resources/clamscan.rc
)

if(MSVC)
    set_target_properties(clamscan PROPERTIES
        VS_MANIFEST_ADDITIONAL_FILES "${CLAMWIN_DIR}/resources/compatibility.manifest")
elseif(MINGW)
    target_compile_definitions(clamscan PRIVATE RC_NEEDS_MANIFEST)
endif()

target_include_directories(clamscan PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamscan PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamscan PRIVATE libclamav_common libclamav ws2_32 iphlpapi psapi)

list(APPEND CLAMAV_INSTALL_TARGETS clamscan)
