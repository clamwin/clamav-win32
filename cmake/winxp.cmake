file(GLOB libclamav_compat_sources
    ${CLAMWIN_DIR}/src/winxp/compat.c
)

add_library(libclamav_compat STATIC
    ${libclamav_compat_sources}
)

set_property(TARGET libclamav_compat PROPERTY
    STATIC_LIBRARY_OPTIONS "-lpsapi"
)

file(GLOB synchapi_sources
    ${CLAMWIN_DIR}/src/winxp/synchapi.c
)

add_library(synchapi SHARED
    ${CLAMWIN_DIR}/src/winxp/synchapi.def
    ${synchapi_sources}
)

set_target_properties(synchapi PROPERTIES PREFIX "" OUTPUT_NAME API-MS-WIN-CORE-SYNCH-L1-2-0)
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

target_link_libraries(libclamav PRIVATE libclamav_compat)
target_link_libraries(libfreshclam PRIVATE libclamav_compat)
target_link_libraries(clambc PRIVATE libclamav_compat)
target_link_libraries(sigtool PRIVATE libclamav_compat)
