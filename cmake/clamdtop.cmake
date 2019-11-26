file(GLOB clamdtop_srcs ${CLAMAV}/clamdtop/*.c)
add_executable(clamdtop
    ${clamdtop_srcs}
    ${CLAMAV}/shared/getopt.c
    ${CLAMAV}/shared/misc.c
    ${CMAKE_SOURCE_DIR}/resources/clamdtop.rc)

target_include_directories(clamdtop PRIVATE ${CLAMAV}/libclamav ${3RDPARTY}/PDCurses)
target_compile_definitions(clamdtop PRIVATE HAVE_CONFIG_H CLAMWIN_MAIN_HANDLED)
target_link_libraries(clamdtop PRIVATE libclamav PDCurses ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamdtop)
