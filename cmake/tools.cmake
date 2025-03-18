list(APPEND TOOLS_DEFINES UNICODE _UNICODE CLAMWIN_MAIN_HANDLED)

add_executable(exeScanner
    ${CLAMWIN_DIR}/tools/exeScanner_app.c
    ${CLAMWIN_DIR}/tools/exeScanner.rc
)

target_link_libraries(exeScanner PRIVATE libclamav_common libclamav ws2_32)
target_include_directories(exeScanner PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(exeScanner PRIVATE ${CLAMWIN_DEFINES} ${TOOLS_DEFINES})
target_compile_options(exeScanner PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

if(NOT ENABLE_LEGACY STREQUAL "win9x")
    add_executable(sigcheck
        ${CLAMWIN_DIR}/tools/sigcheck_app.c
        ${CLAMWIN_DIR}/tools/sigcheck.rc
    )

    target_link_libraries(sigcheck PRIVATE libclamav_common libclamav ws2_32)
    target_include_directories(sigcheck PRIVATE ${CLAMWIN_INCLUDES})
    target_compile_definitions(sigcheck PRIVATE ${CLAMWIN_DEFINES} ${TOOLS_DEFINES})
    target_compile_options(sigcheck PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
endif()

if(MSVC)
    add_executable(unscrambler
        ${CLAMWIN_DIR}/tools/unscrambler.c
        ${CLAMWIN_DIR}/tools/unscrambler.rc
    )

    target_link_options(unscrambler PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_definitions(unscrambler PRIVATE UNICODE)
    target_compile_options(unscrambler PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
endif()
