include(cmake/curl.cmake)

add_library(libfreshclam SHARED
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

target_include_directories(libfreshclam PRIVATE ${CLAMAV}/libclamav ${3RDPARTY}/curl/include)
target_compile_definitions(libfreshclam PRIVATE HAVE_CONFIG_H CURL_STATICLIB)
target_link_libraries(libfreshclam PRIVATE curl libclamav crypt32 ws2_32 iphlpapi)
set_target_properties(libfreshclam PROPERTIES
    PREFIX ""
    OUTPUT_NAME libfreshclam)

file(GLOB freshclam_srcs ${CLAMAV}/freshclam/*.c)
add_executable(freshclam
    ${freshclam_srcs}
    ${CLAMAV}/shared/output.c
    ${CLAMAV}/shared/misc.c
    ${CLAMAV}/shared/clamdcom.c
    ${CMAKE_SOURCE_DIR}/src/helpers/cw_main.c
    ${CMAKE_SOURCE_DIR}/src/helpers/crashdump.c
    ${CMAKE_SOURCE_DIR}/src/helpers/service.c
    ${CMAKE_SOURCE_DIR}/resources/freshclam.rc)

target_include_directories(freshclam PRIVATE ${CLAMAV}/libclamav)
target_compile_definitions(freshclam PRIVATE HAVE_CONFIG_H)
target_link_libraries(freshclam libfreshclam libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS freshclam libfreshclam)
