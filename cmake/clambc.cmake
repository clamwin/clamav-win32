file(GLOB clambc_sources ${CLAMAV_DIR}/clambc/*.c)

set(clambc_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
)

source_group("Win32 Files" FILES ${clambc_win32_sources})

add_executable(clambc
    ${clambc_sources}
    ${clambc_win32_sources}
    ${CLAMWIN_DIR}/resources/clambc.rc
)

target_include_directories(clambc PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clambc PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clambc PRIVATE libclamav_common libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clambc)
