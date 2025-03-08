enable_language(C ASM)

list(APPEND COMPAT_DEFINES UNICODE _WIN32_WINNT=_WIN32_WINNT_WINXP)
option(WINXP_TRACE "Enable support for WINXP" OFF)

if(WINXP_TRACE)
    list(APPEND COMPAT_DEFINES TRACE_COMPAT)
endif()

file(GLOB clamav_compat_sources
    ${CLAMWIN_DIR}/src/winxp/stubs.c
    ${CLAMWIN_DIR}/src/winxp/bcrypt.c
    ${CLAMWIN_DIR}/src/winxp/kernel32.c
    ${CLAMWIN_DIR}/src/winxp/runonce.c
    ${CLAMWIN_DIR}/src/winxp/shell32.c
    ${CLAMWIN_DIR}/src/winxp/forward.S
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

add_executable(sfibh ${CLAMWIN_DIR}/src/winxp/sfibh.c)
target_link_libraries(sfibh PRIVATE clamav_compat psapi ntdll mpr)
target_link_options(sfibh PRIVATE -municode)
target_compile_definitions(sfibh PRIVATE ${COMPAT_DEFINES})
target_compile_options(sfibh PRIVATE -Wall)

add_executable(reopenfile ${CLAMWIN_DIR}/src/winxp/reopenfile.c)
target_link_libraries(reopenfile PRIVATE clamav_compat ntdll)
target_link_options(reopenfile PRIVATE -municode)
target_compile_definitions(reopenfile PRIVATE ${COMPAT_DEFINES})
target_compile_options(reopenfile PRIVATE -Wall)

file(GLOB synchapi_sources
    ${CLAMWIN_DIR}/src/winxp/synchapi.c
    ${CLAMWIN_DIR}/src/winxp/srw.c
    ${CLAMWIN_DIR}/src/winxp/runonce.c
    ${CLAMWIN_DIR}/resources/synchapi.rc
    ${CLAMWIN_DIR}/src/winxp/synchapi.def
)
add_library(synchapi SHARED ${synchapi_sources})
target_compile_definitions(synchapi PRIVATE ${COMPAT_DEFINES})
set_target_properties(synchapi PROPERTIES PREFIX "" OUTPUT_NAME api-ms-win-core-synch-l1-2-0)
list(APPEND CLAMAV_INSTALL_TARGETS synchapi)

file(GLOB bcryptprimitives_sources
    ${CLAMWIN_DIR}/src/winxp/bcryptprimitives.c
    ${CLAMWIN_DIR}/resources/bcryptprimitives.rc
    ${CLAMWIN_DIR}/src/winxp/bcryptprimitives.def
)
add_library(bcryptprimitives SHARED ${bcryptprimitives_sources})
target_compile_definitions(bcryptprimitives PRIVATE ${COMPAT_DEFINES})
target_link_libraries(bcryptprimitives PRIVATE advapi32)
set_target_properties(bcryptprimitives PROPERTIES PREFIX "" OUTPUT_NAME bcryptprimitives)
list(APPEND CLAMAV_INSTALL_TARGETS bcryptprimitives)

target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)
