file(GLOB clamdscan_srcs ${CLAMAV}/clamdscan/*.c)
add_executable(clamdscan
    ${clamdscan_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CLAMAV}/shared/clamdcom.c
    ${CMAKE_SOURCE_DIR}/src/shared/win32actions.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/crashdump.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_sch.c
    ${CMAKE_SOURCE_DIR}/resources/clamdscan.rc)

target_include_directories(clamdscan PRIVATE ${CLAMAV}/libclamav)
target_compile_definitions(clamdscan PRIVATE HAVE_CONFIG_H)
target_link_libraries(clamdscan PRIVATE libclamav ws2_32)
