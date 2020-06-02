file(GLOB clamdscan_headers ${CLAMAV_DIR}/clamdscan/*.h)

file(GLOB clamdscan_sources ${CLAMAV_DIR}/clamdscan/*.c)
list(APPEND clamdscan_sources
    ${CLAMAV_DIR}/shared/output.c
    ${CLAMAV_DIR}/shared/misc.c
    ${CLAMAV_DIR}/shared/clamdcom.c
)

set(clamdscan_win32_sources
    ${CLAMWIN_DIR}/src/shared/win32actions.c
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/src/helpers/cw_sch.c
)

source_group("Win32 Files" FILES ${clamdscan_win32_sources})

add_executable(clamdscan
    ${clamdscan_headers}
    ${clamdscan_sources}
    ${clamdscan_win32_sources}
    ${CLAMWIN_DIR}/resources/clamdscan.rc
)

target_include_directories(clamdscan PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamdscan PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamdscan PRIVATE libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamdscan)
