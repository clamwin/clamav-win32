enable_language(C ASM)

list(APPEND LEGACY_DEFINES ${CLAMWIN_WINNT_VERSION})
option(LEGACY_TRACE "Enable Compatibility Layer TRACE" OFF)

if(LEGACY_TRACE)
    list(APPEND LEGACY_DEFINES LEGACY_TRACE)
endif()

list(APPEND clamav_compat_headers
    ${CLAMWIN_DIR}/src/legacy/shared/legacy.h
)

list(APPEND clamav_compat_sources
    ${CLAMWIN_DIR}/src/legacy/shared/stubs.c
    ${CLAMWIN_DIR}/src/legacy/shared/bcrypt.c
    ${CLAMWIN_DIR}/src/legacy/shared/kernel32.c
    ${CLAMWIN_DIR}/src/legacy/shared/runonce.c
)

if(ENABLE_LEGACY STREQUAL "winxp")
    list(APPEND clamav_compat_sources
        ${CLAMWIN_DIR}/src/legacy/winxp/kernel32.c
    )
    list(APPEND LEGACY_DEFINES UNICODE _UNICODE)
elseif(ENABLE_LEGACY STREQUAL "win9x")
    list(APPEND clamav_compat_sources
        ${CLAMWIN_DIR}/src/legacy/win9x/forward.S
        ${CLAMWIN_DIR}/src/legacy/win9x/rtlcapturecontext.S
        ${CLAMWIN_DIR}/src/legacy/win9x/kernel32.c
    )
endif()

add_library(clamav_compat STATIC
    ${clamav_compat_headers}
    ${clamav_compat_sources}
)

target_include_directories(clamav_compat PRIVATE ${CLAMWIN_DIR}/src/legacy/shared)
target_compile_definitions(clamav_compat PRIVATE ${LEGACY_DEFINES})
target_compile_options(clamav_compat PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>)
target_compile_options(clamav_compat PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4061 /wd4820>)

# test tools
if(ENABLE_LEGACY STREQUAL "winxp")
    add_executable(gfpn ${CLAMWIN_DIR}/src/legacy/tests/gfpn.c)
    target_compile_definitions(gfpn PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(gfpn PRIVATE clamav_compat psapi ntdll mpr)
    target_link_options(gfpn PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(gfpn PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

    add_executable(sfibh ${CLAMWIN_DIR}/src/legacy/tests/sfibh.c)
    target_compile_definitions(sfibh PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(sfibh PRIVATE clamav_compat psapi ntdll mpr)
    target_link_options(sfibh PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(sfibh PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
    target_include_directories(sfibh PRIVATE ${CLAMWIN_DIR}/src/legacy/shared)

    add_executable(reopenfile ${CLAMWIN_DIR}/src/legacy/tests/reopenfile.c)
    target_compile_definitions(reopenfile PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(reopenfile PRIVATE clamav_compat ntdll)
    target_link_options(reopenfile PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(reopenfile PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
elseif(ENABLE_LEGACY STREQUAL "win9x")
    # link unicows
    find_library(UNICOWS_LIBRARY
        NAMES libunicows.a unicows libunicows
        HINTS ${3RDPARTY_DIR}/libunicows
        REQUIRED
        NO_DEFAULT_PATH)
    target_link_libraries(libclamav PRIVATE ${UNICOWS_LIBRARY})
    target_link_libraries(libclamunrar PRIVATE ${UNICOWS_LIBRARY})

    # userenv
    get_target_property(CLAMV_RUST_LIBS clamav_rust INTERFACE_LINK_LIBRARIES)
    list(REMOVE_ITEM CLAMV_RUST_LIBS -luserenv userenv)
    set_target_properties(clamav_rust PROPERTIES INTERFACE_LINK_LIBRARIES "${CLAMV_RUST_LIBS}")
    target_sources(clamav_compat PRIVATE ${CLAMWIN_DIR}/src/legacy/win9x/userenv.c)
endif()

# api-ms-win-core-synch-l1-2-0.dll
list(APPEND synchapi_sources
    ${CLAMWIN_DIR}/src/legacy/shared/runonce.c
    ${CLAMWIN_DIR}/src/legacy/shared/synchapi.c
    ${CLAMWIN_DIR}/src/legacy/shared/synchapi.def
    ${CLAMWIN_DIR}/resources/synchapi.rc
)

add_library(synchapi SHARED ${synchapi_sources})
target_compile_definitions(synchapi PRIVATE ${LEGACY_DEFINES})
target_compile_options(synchapi PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>)
target_link_options(synchapi PRIVATE $<$<C_COMPILER_ID:MSVC>:/FORCE:MULTIPLE>)
set_target_properties(synchapi PROPERTIES PREFIX "" OUTPUT_NAME api-ms-win-core-synch-l1-2-0)
list(APPEND CLAMAV_INSTALL_TARGETS synchapi)

# bcryptprimitives.dll
list(APPEND bcryptprimitives_sources
    ${CLAMWIN_DIR}/src/legacy/shared/bcryptprimitives.c
    ${CLAMWIN_DIR}/src/legacy/shared/bcryptprimitives.def
    ${CLAMWIN_DIR}/resources/bcryptprimitives.rc
)
add_library(bcryptprimitives SHARED ${bcryptprimitives_sources})
target_compile_definitions(bcryptprimitives PRIVATE ${LEGACY_DEFINES})
target_compile_options(clamav_compat PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>)
target_link_libraries(bcryptprimitives PRIVATE advapi32)
set_target_properties(bcryptprimitives PROPERTIES PREFIX "" OUTPUT_NAME bcryptprimitives)
list(APPEND CLAMAV_INSTALL_TARGETS bcryptprimitives)

# "taint" needy executables
target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)

if(ENABLE_LEGACY STREQUAL "win9x")
    target_link_libraries(clamd PRIVATE clamav_compat)
    target_link_libraries(clamscan PRIVATE clamav_compat)
    target_link_libraries(clamdscan PRIVATE clamav_compat)
    target_link_libraries(clamdtop PRIVATE clamav_compat)
    target_link_libraries(libclamunrar PRIVATE clamav_compat)
endif()

target_link_options(libclamav PRIVATE $<$<C_COMPILER_ID:MSVC>:/FORCE:MULTIPLE>)
