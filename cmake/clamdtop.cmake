file(GLOB clamdtop_sources ${CLAMAV_DIR}/clamdtop/*.c)

add_executable(clamdtop
    ${clamdtop_sources}
    ${CLAMWIN_DIR}/resources/clamdtop.rc
)

target_include_directories(clamdtop PRIVATE ${CLAMWIN_INCLUDES} ${3RDPARTY_DIR}/PDCurses)
target_compile_definitions(clamdtop PRIVATE ${CLAMWIN_DEFINES} CLAMWIN_MAIN_HANDLED)
target_link_libraries(clamdtop PRIVATE libclamav libclamav_common PDCurses ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamdtop)
