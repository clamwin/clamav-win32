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

if(CLAMWIN_WINDOWS_VERSION LESS_EQUAL 0x0501 AND CLAMAV_ARCH STREQUAL "x86")
    list(APPEND clamav_compat_sources
        ${CLAMWIN_DIR}/src/legacy/4.0/forward.S
        ${CLAMWIN_DIR}/src/legacy/4.0/rtlcapturecontext.S
        ${CLAMWIN_DIR}/src/legacy/4.0/kernel32.c
    )
endif()

add_library(clamav_compat STATIC
    ${clamav_compat_headers}
    ${clamav_compat_sources}
)

set(LEGACY_INCLUDES
    ${CLAMWIN_DIR}/src/legacy/shared
    ${CLAMWIN_DIR}/include
)

target_include_directories(clamav_compat PRIVATE ${LEGACY_INCLUDES})
target_compile_definitions(clamav_compat PRIVATE ${LEGACY_DEFINES})
target_compile_options(clamav_compat PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>)
target_compile_options(clamav_compat PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4061 /wd4273 /wd4820>)
target_link_libraries(clamav_compat INTERFACE ntdll)

function(add_legacy_executable TARGET SOURCES LINK_LIBRARY)
    add_executable(${TARGET} ${SOURCES})
    target_include_directories(${TARGET} PRIVATE ${LEGACY_INCLUDES})
    target_compile_definitions(${TARGET} PRIVATE ${LEGACY_DEFINES})
    target_link_libraries(${TARGET} PRIVATE ${LINK_LIBRARY})
    target_link_options(${TARGET} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)
    target_compile_options(${TARGET} PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/wd4061 /wd4273>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>
    )
endfunction()

# test tools
if(ENABLE_TEST_TOOLS)
    add_legacy_executable(gfpn ${CLAMWIN_DIR}/src/legacy/tests/gfpn.c clamav_compat)
    add_legacy_executable(gfpn-native ${CLAMWIN_DIR}/src/legacy/tests/gfpn.c ntdll)
    add_legacy_executable(glpn ${CLAMWIN_DIR}/src/legacy/tests/glpn.c clamav_compat)
    add_legacy_executable(glpn-native ${CLAMWIN_DIR}/src/legacy/tests/glpn.c ntdll)
    add_legacy_executable(sfibh ${CLAMWIN_DIR}/src/legacy/tests/sfibh.c clamav_compat)
    add_legacy_executable(reopenfile ${CLAMWIN_DIR}/src/legacy/tests/reopenfile.c clamav_compat)
endif()

if(CLAMWIN_WINDOWS_VERSION LESS_EQUAL 0x0501 AND CLAMAV_ARCH STREQUAL "x86")
    # userenv
    get_target_property(CLAMV_RUST_LIBS clamav_rust INTERFACE_LINK_LIBRARIES)
    list(REMOVE_ITEM CLAMV_RUST_LIBS -luserenv userenv)
    set_target_properties(clamav_rust PROPERTIES INTERFACE_LINK_LIBRARIES "${CLAMV_RUST_LIBS}")
    target_sources(clamav_compat PRIVATE ${CLAMWIN_DIR}/src/legacy/4.0/userenv.c)
endif()

install(FILES ${CLAMWIN_DIR}/src/legacy/LICENSE.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME legacy.txt)

# "taint" needy executables
target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)

if(CLAMWIN_WINDOWS_VERSION LESS_EQUAL 0x0501 AND CLAMAV_ARCH STREQUAL "x86")
    target_link_libraries(freshclam PRIVATE clamav_compat)
    target_link_libraries(clamscan PRIVATE clamav_compat)
    target_link_libraries(libclamunrar PRIVATE clamav_compat)
    target_link_libraries(clamd PRIVATE clamav_compat)
    target_link_libraries(clamdscan PRIVATE clamav_compat)
    target_link_libraries(clamdtop PRIVATE clamav_compat)
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    get_target_property(RUST_ARCHIVE clamav_rust IMPORTED_LOCATION)
    set(RUST_FILTERED_ARCHIVE "${RUST_ARCHIVE}.filtered")

    add_custom_command(
        OUTPUT "${RUST_FILTERED_ARCHIVE}"
        COMMAND ${CMAKE_COMMAND} -E copy "${RUST_ARCHIVE}" "${RUST_FILTERED_ARCHIVE}"
        COMMAND ${CMAKE_AR} t "${RUST_FILTERED_ARCHIVE}" > filelist.txt
        COMMAND ${CMAKE_COMMAND} -E env bash -c 'for f in $$$(grep -E "api-ms-win-core-synch-l1-2-0\\|bcryptprimitives\\|kernel32.dlls00000.o" filelist.txt) \; do ${CMAKE_AR} d ${RUST_FILTERED_ARCHIVE} $$f \; done'
        DEPENDS "${RUST_ARCHIVE}"
    )

    add_custom_target(filter_clamav_rust DEPENDS "${RUST_FILTERED_ARCHIVE}")
    set_target_properties(clamav_rust PROPERTIES IMPORTED_LOCATION "${RUST_FILTERED_ARCHIVE}")
    add_dependencies(libclamav filter_clamav_rust clamav_rust)
    add_dependencies(libfreshclam filter_clamav_rust clamav_rust)
    add_dependencies(sigtool filter_clamav_rust clamav_rust)
    add_dependencies(clambc filter_clamav_rust clamav_rust)
endif()

target_link_options(libclamav PRIVATE $<$<C_COMPILER_ID:MSVC>:/FORCE:MULTIPLE>)
