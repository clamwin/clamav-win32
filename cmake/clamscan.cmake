file(GLOB clamscan_headers ${CLAMAV_DIR}/clamscan/*.h)

file(GLOB clamscan_sources ${CLAMAV_DIR}/clamscan/*.c)
list(APPEND clamscan_sources
    ${CLAMAV_DIR}/shared/output.c
    ${CLAMAV_DIR}/shared/misc.c
)

set(clamscan_win32_sources
    ${CLAMWIN_DIR}/src/shared/win32actions.c
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/src/helpers/scanmem.c
    ${CLAMWIN_DIR}/src/helpers/exeScanner.c
)

source_group("Win32 Files" FILES ${clamscan_win32_sources})

add_executable(clamscan
    ${clamscan_headers}
    ${clamscan_sources}
    ${clamscan_win32_sources}
    ${CLAMWIN_DIR}/resources/clamscan.rc
)

target_include_directories(clamscan PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamscan PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamscan PRIVATE libclamav ws2_32 iphlpapi)

list(APPEND CLAMAV_INSTALL_TARGETS clamscan)
