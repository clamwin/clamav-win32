include(cmake/curl.cmake)

file(GLOB libfreshclam_headers ${CLAMAV_DIR}/libfreshclam/*.h)

set(libfreshclam_sources
    ${CLAMAV_DIR}/libfreshclam/dns.c
    ${CLAMAV_DIR}/libfreshclam/libfreshclam.c
    ${CLAMAV_DIR}/libfreshclam/libfreshclam_internal.c
)

set(libfreshclam_win32_sources
    ${CLAMWIN_DIR}/resources/libfreshclam.rc
    ${CLAMWIN_DIR}/libfreshclam.def
)

if(ENABLE_LEGACY STREQUAL "win9x")
    list(APPEND libfreshclam_win32_sources ${CLAMWIN_DIR}/src/legacy/win9x/resolv.c)
else()
    list(APPEND libfreshclam_win32_sources ${CLAMAV_DIR}/win32/compat/resolv.c)
endif()

file(GLOB libfreshclam_common_sources
    ${CLAMAV_DIR}/common/cert_util.c
    ${CLAMAV_DIR}/common/win/cert_util_win.c
)

source_group("Win32 Files" FILES ${libfreshclam_win32_sources})

add_library(libfreshclam SHARED
    ${libfreshclam_headers}
    ${libfreshclam_sources}
    ${libfreshclam_win32_sources}
    ${libfreshclam_common_sources}
)

set_target_properties(libfreshclam PROPERTIES PREFIX "" OUTPUT_NAME libfreshclam)
target_include_directories(libfreshclam PRIVATE ${CLAMWIN_INCLUDES} ${3RDPARTY_DIR}/curl/include)
target_compile_definitions(libfreshclam PRIVATE ${CLAMWIN_DEFINES} CURL_STATICLIB)
target_link_libraries(libfreshclam PRIVATE
    libcurl_static
    zlibstatic
    libclamav_common
    libclamav
    crypt32
    ws2_32
    iphlpapi)

if(NOT ENABLE_LEGACY STREQUAL "win9x")
    target_link_libraries(libfreshclam PRIVATE dnsapi)
endif()

# freshclam
file(GLOB freshclam_headers ${CLAMAV_DIR}/freshclam/*.h)
file(GLOB freshclam_sources ${CLAMAV_DIR}/freshclam/*.c)

set(freshclam_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/resources/freshclam.rc
)

source_group("Win32 Files" FILES ${freshclam_win32_sources})

add_executable(freshclam
    ${freshclam_headers}
    ${freshclam_sources}
    ${freshclam_win32_sources}
)

target_include_directories(freshclam PRIVATE ${CLAMWIN_INCLUDES} ${CLAMAV_DIR}/libfreshclam)
target_compile_definitions(freshclam PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(freshclam libfreshclam libclamav_common libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS freshclam libfreshclam)
