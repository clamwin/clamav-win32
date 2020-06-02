file(GLOB clamd_headers ${CLAMAV_DIR}/clamd/*.h)

file(GLOB clamd_sources ${CLAMAV_DIR}/clamd/*.c)
list(APPEND clamd_sources
    ${CLAMAV_DIR}/shared/output.c
    ${CLAMAV_DIR}/shared/misc.c
    ${CLAMAV_DIR}/shared/clamdcom.c
    ${CLAMAV_DIR}/shared/idmef_logging.c
)

set(clamd_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/src/helpers/service.c
    ${CLAMWIN_DIR}/src/helpers/win32poll.c
)

source_group("Win32 Files" FILES ${clamd_win32_sources})

add_executable(clamd
    ${clamd_headers}
    ${clamd_sources}
    ${clamd_win32_sources}
    ${CLAMWIN_DIR}/resources/clamd.rc
)

target_include_directories(clamd PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamd PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamd PRIVATE libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamd)
