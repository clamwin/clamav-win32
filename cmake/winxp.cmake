file(GLOB clamav_compat_sources
    ${CLAMWIN_DIR}/src/winxp/stubs.c
    ${CLAMWIN_DIR}/src/winxp/advapi32.c
    ${CLAMWIN_DIR}/src/winxp/kernel32.c
    ${CLAMWIN_DIR}/src/winxp/shell32.c
)

add_library(clamav_compat STATIC
    ${clamav_compat_sources}
)

file(GLOB synchapi_sources
    ${CLAMWIN_DIR}/src/winxp/synchapi.c
    ${CLAMWIN_DIR}/resources/synchapi.rc
)

add_library(synchapi SHARED
    ${CLAMWIN_DIR}/src/winxp/synchapi.def
    ${synchapi_sources}
)

set_target_properties(synchapi PROPERTIES PREFIX "" OUTPUT_NAME api-ms-win-core-synch-l1-2-0)
list(APPEND CLAMAV_INSTALL_TARGETS synchapi)

file(GLOB bcryptprimitives_sources
    ${CLAMWIN_DIR}/src/winxp/bcryptprimitives.c
)

add_library(bcryptprimitives SHARED
    ${CLAMWIN_DIR}/src/winxp/bcryptprimitives.def
    ${bcryptprimitives_sources}
)

target_link_libraries(bcryptprimitives PRIVATE advapi32)

set_target_properties(bcryptprimitives PROPERTIES PREFIX "" OUTPUT_NAME bcryptprimitives)
list(APPEND CLAMAV_INSTALL_TARGETS bcryptprimitives)

target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)
