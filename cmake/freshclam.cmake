include(cmake/curl.cmake)

add_library(freshclam SHARED
    ${CLAMAV}/libfreshclam/libfreshclam.c
    ${CLAMAV}/libfreshclam/libfreshclam_internal.c
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CLAMAV}/shared/tar.c
    ${CLAMAV}/shared/cdiff.c
    ${CLAMAV}/shared/cert_util.c
    ${CLAMAV}/shared/win/cert_util_win.c
    ${CMAKE_SOURCE_DIR}/src/helpers/dnsquery.c
    ${CMAKE_SOURCE_DIR}/resources/libfreshclam.rc
    ${CMAKE_SOURCE_DIR}/libfreshclam.def)

target_link_libraries(freshclam PRIVATE curl clamav crypt32 ws2_32 iphlpapi)
target_compile_definitions(freshclam PRIVATE HAVE_CONFIG_H CURL_STATICLIB)
target_include_directories(freshclam PRIVATE ${CLAMAV}/libclamav ${3RDPARTY}/curl/include)

file(GLOB freshclam_srcs ${CLAMAV}/freshclam/*.c)

add_executable(freshclam-bin
    ${freshclam_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CLAMAV}/shared/clamdcom.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/service.c
    ${CMAKE_SOURCE_DIR}/resources/freshclam.rc)

set_target_properties(freshclam-bin PROPERTIES OUTPUT_NAME freshclam)
target_link_libraries(freshclam-bin freshclam clamav ws2_32)
target_compile_definitions(freshclam-bin PRIVATE HAVE_CONFIG_H)
target_include_directories(freshclam-bin PRIVATE ${CLAMAV}/libclamav)
