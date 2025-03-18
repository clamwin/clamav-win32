file(GLOB clamdscan_headers ${CLAMAV_DIR}/clamdscan/*.h)
file(GLOB clamdscan_sources ${CLAMAV_DIR}/clamdscan/*.c)

set(clamdscan_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/resources/clamdscan.rc
)

source_group("Win32 Files" FILES ${clamdscan_win32_sources})

add_executable(clamdscan
    ${clamdscan_headers}
    ${clamdscan_sources}
    ${clamdscan_win32_sources}
)

target_include_directories(clamdscan PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamdscan PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamdscan PRIVATE libclamav_common libclamav ws2_32 psapi)

list(APPEND CLAMAV_INSTALL_TARGETS clamdscan)
