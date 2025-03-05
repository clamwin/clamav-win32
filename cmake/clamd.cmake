file(GLOB clamd_headers ${CLAMAV_DIR}/clamd/*.h)
file(GLOB clamd_sources ${CLAMAV_DIR}/clamd/*.c)

set(clamd_win32_sources
    ${CLAMWIN_DIR}/src/helpers/cw_main.c
    ${CLAMWIN_DIR}/src/helpers/crashdump.c
    ${CLAMAV_DIR}/win32/compat/net.c
)

source_group("Win32 Files" FILES ${clamd_win32_sources})

add_executable(clamd
    ${clamd_headers}
    ${clamd_sources}
    ${clamd_win32_sources}
    ${CLAMWIN_DIR}/resources/clamd.rc
)

# clamav/win32/compat/net.c needs poll api
set(CLAMD_DEFINES ${CLAMWIN_DEFINES})
list(FILTER CLAMD_DEFINES EXCLUDE REGEX "^_WIN32_WINNT=.*$")
list(APPEND CLAMD_DEFINES _WIN32_WINNT=0x0600)

if(MSVC)
    target_sources(clamd PRIVATE ${CLAMWIN_DIR}/resources/compatibility.manifest)
elseif(MINGW)
    target_compile_definitions(clamd PRIVATE RC_NEEDS_MANIFEST)
endif()

target_include_directories(clamd PRIVATE ${CLAMWIN_INCLUDES})
target_compile_definitions(clamd PRIVATE ${CLAMD_DEFINES})
target_link_libraries(clamd PRIVATE libclamav_common libclamav ws2_32)

list(APPEND CLAMAV_INSTALL_TARGETS clamd)
