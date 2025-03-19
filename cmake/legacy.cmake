enable_language(C ASM)

list(APPEND LEGACY_DEFINES ${CLAMWIN_WINNT_VERSION} ${UNICODE_DEFINES})
option(LEGACY_TRACE "Enable Compatibility Layer TRACE" OFF)

if(LEGACY_TRACE)
    list(APPEND LEGACY_DEFINES LEGACY_TRACE)
endif()

list(APPEND clamav_compat_headers
    ${CLAMWIN_DIR}/src/legacy/shared/legacy.h
)

file(GLOB clamav_compat_sources
    ${CLAMWIN_DIR}/src/legacy/shared/*.c
    ${CLAMWIN_DIR}/src/legacy/shared/forward.S
)

if(CLAMWIN_WINDOWS_VERSION EQUAL 0x0501)
    list(APPEND clamav_compat_sources
        ${CLAMWIN_DIR}/src/legacy/winxp/kernel32.c
    )
elseif(CLAMWIN_WINDOWS_VERSION LESS 0x0501)
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

target_include_directories(clamav_compat PRIVATE ${CLAMWIN_DIR}/src/legacy/shared ${CLAMWIN_DIR}/include)
target_compile_definitions(clamav_compat PRIVATE ${LEGACY_DEFINES})
target_compile_options(clamav_compat PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>)
target_compile_options(clamav_compat PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4061 /wd4273 /wd4820>)

# test tools
if(CLAMWIN_WINDOWS_VERSION GREATER_EQUAL 0x0501)
    add_executable(gfpn ${CLAMWIN_DIR}/src/legacy/tests/gfpn.c)
    target_compile_definitions(gfpn PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(gfpn PRIVATE clamav_compat ntdll)
    target_link_options(gfpn PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(gfpn PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

    add_executable(sfibh ${CLAMWIN_DIR}/src/legacy/tests/sfibh.c)
    target_compile_definitions(sfibh PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(sfibh PRIVATE clamav_compat)
    target_link_options(sfibh PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(sfibh PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
    target_include_directories(sfibh PRIVATE ${CLAMWIN_DIR}/src/legacy/shared)

    add_executable(reopenfile ${CLAMWIN_DIR}/src/legacy/tests/reopenfile.c)
    target_compile_definitions(reopenfile PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(reopenfile PRIVATE clamav_compat ntdll)
    target_link_options(reopenfile PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(reopenfile PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
endif()

if(NOT CLAMWIN_UNICODE_BUILD)
    # link unicows
    find_library(UNICOWS_LIBRARY
        NAMES libunicows.a unicows libunicows
        HINTS ${3RDPARTY_DIR}/libunicows
        REQUIRED
        NO_DEFAULT_PATH)
    target_link_libraries(libclamav PRIVATE ${UNICOWS_LIBRARY})
    target_link_libraries(libclamunrar PRIVATE ${UNICOWS_LIBRARY})
endif()

if(CLAMWIN_WINDOWS_VERSION LESS 0x0501)
    # userenv
    get_target_property(CLAMV_RUST_LIBS clamav_rust INTERFACE_LINK_LIBRARIES)
    list(REMOVE_ITEM CLAMV_RUST_LIBS -luserenv userenv)
    set_target_properties(clamav_rust PROPERTIES INTERFACE_LINK_LIBRARIES "${CLAMV_RUST_LIBS}")
    target_sources(clamav_compat PRIVATE ${CLAMWIN_DIR}/src/legacy/win9x/userenv.c)
endif()

# "taint" needy executables
target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)

if(CLAMWIN_WINDOWS_VERSION LESS 0x0501)
    target_link_libraries(clamd PRIVATE clamav_compat)
    target_link_libraries(clamscan PRIVATE clamav_compat)
    target_link_libraries(clamdscan PRIVATE clamav_compat)
    target_link_libraries(clamdtop PRIVATE clamav_compat)
    target_link_libraries(libclamunrar PRIVATE clamav_compat)
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    get_target_property(RUST_ARCHIVE clamav_rust IMPORTED_LOCATION)
    set(RUST_FILTERED_ARCHIVE "${RUST_ARCHIVE}.filtered")

    add_custom_command(
        OUTPUT "${RUST_FILTERED_ARCHIVE}"
        COMMAND ${CMAKE_COMMAND} -E copy "${RUST_ARCHIVE}" "${RUST_FILTERED_ARCHIVE}"
        COMMAND ${CMAKE_AR} t "${RUST_FILTERED_ARCHIVE}" > filelist.txt
        COMMAND ${CMAKE_COMMAND} -E env bash -c 'for f in $$$(grep -E "api-ms-win-core-synch-l1-2-0\\|bcryptprimitives" filelist.txt) \; do ${CMAKE_AR} d ${RUST_FILTERED_ARCHIVE} $$f \; done'
        DEPENDS "${RUST_ARCHIVE}"
    )

    add_custom_target(filter_clamav_rust DEPENDS "${RUST_FILTERED_ARCHIVE}")
    set_target_properties(clamav_rust PROPERTIES IMPORTED_LOCATION "${RUST_FILTERED_ARCHIVE}")
    add_dependencies(libclamav filter_clamav_rust)
endif()

target_link_options(libclamav PRIVATE $<$<C_COMPILER_ID:MSVC>:/FORCE:MULTIPLE>)
