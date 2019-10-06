file(GLOB sigtool_srcs ${CLAMAV}/sigtool/*.c)
add_executable(sigtool
    ${sigtool_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CLAMAV}/shared/tar.c
    ${CLAMAV}/shared/cdiff.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/crashdump.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_sch.c
    ${CMAKE_SOURCE_DIR}/resources/sigtool.rc)

target_include_directories(sigtool PRIVATE ${CLAMAV}/libclamav)
target_compile_definitions(sigtool PRIVATE HAVE_CONFIG_H)
target_link_libraries(sigtool PRIVATE libclamav ws2_32)
