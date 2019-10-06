file(GLOB clambc_srcs ${CLAMAV}/clambc/*.c)
add_executable(clambc
    ${clambc_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/crashdump.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_sch.c
    ${CMAKE_SOURCE_DIR}/resources/clambc.rc)

target_include_directories(clambc PRIVATE ${CLAMAV}/libclamav)
target_compile_definitions(clambc PRIVATE HAVE_CONFIG_H)
target_link_libraries(clambc PRIVATE libclamav ws2_32)
