set(clamsubmit_sources
    ${CLAMAV_DIR}/clamsubmit/clamsubmit.c
    ${CLAMAV_DIR}/common/cert_util.c
    ${CLAMAV_DIR}/common/win/cert_util_win.c
)

set(clamsubmit_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/resources/clamsubmit.rc
)

source_group("Win32 Files" FILES ${clamsubmit_win32_sources})

add_executable(clamsubmit
    ${clamsubmit_headers}
    ${clamsubmit_sources}
    ${clamsubmit_win32_sources}
)

target_include_directories(clamsubmit PRIVATE ${CLAMWIN_INCLUDES} ${3RDPARTY_DIR}/curl/include)
target_compile_definitions(clamsubmit PRIVATE ${CLAMWIN_DEFINES} CURL_STATICLIB)
target_link_libraries(clamsubmit PRIVATE json-c libcurl_static libclamav_common libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamsubmit)
