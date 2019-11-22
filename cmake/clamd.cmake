file(GLOB clamd_srcs ${CLAMAV}/clamd/*.c)
add_executable(clamd
    ${clamd_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CLAMAV}/shared/clamdcom.c
    ${CLAMAV}/shared/idmef_logging.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/crashdump.c
    ${CMAKE_SOURCE_DIR}/src/helpers/service.c
    ${CMAKE_SOURCE_DIR}/src/helpers/win32poll.c
    ${CMAKE_SOURCE_DIR}/resources/clamd.rc)

target_include_directories(clamd PRIVATE ${CLAMAV}/libclamav)
target_compile_definitions(clamd PRIVATE HAVE_CONFIG_H)
target_link_libraries(clamd PRIVATE libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamd)
