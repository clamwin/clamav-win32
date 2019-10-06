file(GLOB clamscan_srcs ${CLAMAV}/clamscan/*.c)
add_executable(clamscan
    ${clamscan_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CMAKE_SOURCE_DIR}/src/shared/win32actions.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/crashdump.c
    ${CMAKE_SOURCE_DIR}/src/helpers/scanmem.c
    ${CMAKE_SOURCE_DIR}/src/helpers/exeScanner.c
    ${CMAKE_SOURCE_DIR}/resources/clamscan.rc)

target_include_directories(clamscan PRIVATE ${CLAMAV}/libclamav)
target_compile_definitions(clamscan PRIVATE HAVE_CONFIG_H)
target_link_libraries(clamscan PRIVATE libclamav ws2_32 iphlpapi)
