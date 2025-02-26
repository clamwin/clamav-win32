list(APPEND COMPAT_DEFINES
    _WIN32_WINNT=0x0501
    TRACE_COMPAT
)

file(GLOB clamav_compat_sources
    ${CLAMWIN_DIR}/src/winxp/stubs.c
    ${CLAMWIN_DIR}/src/winxp/advapi32.c
    ${CLAMWIN_DIR}/src/winxp/kernel32.c
    ${CLAMWIN_DIR}/src/winxp/runonce.c
    ${CLAMWIN_DIR}/src/winxp/shell32.c
)

add_library(clamav_compat STATIC
    ${clamav_compat_sources}
)
target_compile_definitions(clamav_compat PRIVATE ${COMPAT_DEFINES})
target_compile_options(clamav_compat PRIVATE -Wall)

add_executable(gfpn ${CLAMWIN_DIR}/src/winxp/gfpn.c)
target_link_libraries(gfpn PRIVATE clamav_compat psapi ntdll mpr)
target_link_options(gfpn PRIVATE -municode)
target_compile_definitions(gfpn PRIVATE ${COMPAT_DEFINES})
target_compile_options(gfpn PRIVATE -Wall)

file(GLOB synchapi_sources
    ${CLAMWIN_DIR}/src/winxp/synchapi.c
    ${CLAMWIN_DIR}/resources/synchapi.rc
)

add_library(synchapi SHARED
    ${CLAMWIN_DIR}/src/winxp/synchapi.def
    ${synchapi_sources}
)
target_compile_definitions(synchapi PRIVATE ${COMPAT_DEFINES})

set_target_properties(synchapi PROPERTIES PREFIX "" OUTPUT_NAME api-ms-win-core-synch-l1-2-0)
list(APPEND CLAMAV_INSTALL_TARGETS synchapi)

file(GLOB bcryptprimitives_sources
    ${CLAMWIN_DIR}/src/winxp/bcryptprimitives.c
)

add_library(bcryptprimitives SHARED
    ${CLAMWIN_DIR}/src/winxp/bcryptprimitives.def
    ${bcryptprimitives_sources}
)
target_compile_definitions(bcryptprimitives PRIVATE ${COMPAT_DEFINES})

target_link_libraries(bcryptprimitives PRIVATE advapi32)

set_target_properties(bcryptprimitives PROPERTIES PREFIX "" OUTPUT_NAME bcryptprimitives)
list(APPEND CLAMAV_INSTALL_TARGETS bcryptprimitives)

target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)
