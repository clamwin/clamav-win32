file(GLOB clamdscan_headers ${CLAMAV_DIR}/clamdscan/*.h)
file(GLOB clamdscan_sources ${CLAMAV_DIR}/clamdscan/*.c)

set(clamdscan_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
)

source_group("Win32 Files" FILES ${clamdscan_win32_sources})

add_executable(clamdscan
    ${clamdscan_headers}
    ${clamdscan_sources}
    ${clamdscan_win32_sources}
    ${CLAMWIN_DIR}/resources/clamdscan.rc
)

if(MSVC)
    set_target_properties(clamdscan PROPERTIES
        VS_MANIFEST_ADDITIONAL_FILES "${CLAMWIN_DIR}/resources/compatibility.manifest")
elseif(MINGW)
    target_compile_definitions(clamdscan PRIVATE RC_NEEDS_MANIFEST)
endif()

target_include_directories(clamdscan PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamdscan PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamdscan PRIVATE libclamav_common libclamav psapi ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamdscan)
