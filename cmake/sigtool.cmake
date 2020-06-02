file(GLOB sigtool_headers ${CLAMAV_DIR}/sigtool/*.h)

file(GLOB sigtool_sources ${CLAMAV_DIR}/sigtool/*.c)
list(APPEND sigtool_sources
    ${CLAMAV_DIR}/shared/output.c
    ${CLAMAV_DIR}/shared/misc.c
    ${CLAMAV_DIR}/shared/tar.c
    ${CLAMAV_DIR}/shared/cdiff.c
)

set(sigtool_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/src/helpers/cw_sch.c
)

source_group("Win32 Files" FILES ${sigtool_win32_sources})

add_executable(sigtool
    ${sigtool_headers}
    ${sigtool_sources}
    ${sigtool_win32_sources}
    ${CLAMWIN_DIR}/resources/sigtool.rc
)

target_include_directories(sigtool PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(sigtool PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(sigtool PRIVATE libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS sigtool)
