file(GLOB clamd_headers ${CLAMAV_DIR}/clamd/*.h)
file(GLOB clamd_sources ${CLAMAV_DIR}/clamd/*.c)

set(clamd_win32_sources
    ${CLAMAV_DIR}/win32/compat/net.c
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMWIN_DIR}/src/helpers/mallinfo.c
    ${CLAMWIN_DIR}/resources/clamd.rc
)

source_group("Win32 Files" FILES ${clamd_win32_sources})

add_executable(clamd
    ${clamd_headers}
    ${clamd_sources}
    ${clamd_win32_sources}
)

set_source_files_properties(
    ${CLAMAV_DIR}/clamd/clamd.c
    PROPERTIES COMPILE_DEFINITIONS "${UNICODE_DEFINES}"
)

if(MSVC)
    target_sources(clamd PRIVATE ${CLAMWIN_DIR}/resources/compatibility.manifest)
elseif(MINGW)
    target_compile_definitions(clamd PRIVATE RC_NEEDS_MANIFEST)
endif()

target_include_directories(clamd PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamd PRIVATE ${CLAMWIN_DEFINES})
target_link_libraries(clamd PRIVATE libclamav_common libclamav ws2_32 psapi)

list(APPEND CLAMAV_INSTALL_TARGETS clamd)
