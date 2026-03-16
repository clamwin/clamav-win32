enable_language(C ASM)

list(APPEND LEGACY_DEFINES ${CLAMWIN_WINNT_VERSION})
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

list(APPEND clamav_compat_sources
    ${CLAMWIN_DIR}/src/legacy/4.0/forward.S
    ${CLAMWIN_DIR}/src/legacy/4.0/rtlcapturecontext.S
    ${CLAMWIN_DIR}/src/legacy/4.0/kernel32.c
)

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

# melt in opencow
file(GLOB opencow_headers ${CLAMWIN_DIR}/src/legacy/opencow/*.h)
file(GLOB opencow_sources
    ${CLAMWIN_DIR}/src/legacy/opencow/*.c
    ${CLAMWIN_DIR}/src/legacy/opencow/*.cpp
    ${CLAMWIN_DIR}/src/legacy/opencow/forward.S
)
add_library(opencow STATIC ${opencow_headers} ${opencow_sources})
target_include_directories(opencow PRIVATE ${LEGACY_INCLUDES})
target_compile_options(opencow PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-attributes>)
target_link_options(opencow PRIVATE $<$<C_COMPILER_ID:MSVC>:/FORCE:MULTIPLE>)

target_link_libraries(libclamav PRIVATE opencow)
target_link_libraries(libfreshclam PRIVATE opencow)
target_link_libraries(libclamunrar PRIVATE opencow)
target_link_libraries(libclamunrar_iface PRIVATE opencow)

target_link_libraries(clamscan PRIVATE opencow)
target_link_libraries(sigtool PRIVATE opencow)
target_link_libraries(freshclam PRIVATE opencow)
install(FILES ${CLAMWIN_DIR}/src/legacy/opencow/LICENCE.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME opencow.txt)

# userenv
get_target_property(CLAMV_RUST_LIBS clamav_rust INTERFACE_LINK_LIBRARIES)
list(REMOVE_ITEM CLAMV_RUST_LIBS -luserenv userenv)
set_target_properties(clamav_rust PROPERTIES INTERFACE_LINK_LIBRARIES "${CLAMV_RUST_LIBS}")
target_sources(clamav_compat PRIVATE ${CLAMWIN_DIR}/src/legacy/4.0/userenv.c)

install(FILES ${CLAMWIN_DIR}/src/legacy/LICENSE.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/copyright RENAME legacy.txt)

# "taint" needy executables
target_link_libraries(libclamav PRIVATE clamav_compat)
target_link_libraries(libfreshclam PRIVATE clamav_compat)
target_link_libraries(clambc PRIVATE clamav_compat)
target_link_libraries(sigtool PRIVATE clamav_compat)

target_link_libraries(freshclam PRIVATE clamav_compat)
target_link_libraries(clamscan PRIVATE clamav_compat)
target_link_libraries(libclamunrar PRIVATE clamav_compat)

if(NOT DEFINED Python3_EXECUTABLE)
    find_package(Python3 COMPONENTS Interpreter QUIET)
endif()

set(_RUST_FILTER_PY_CMD)
if(Python3_EXECUTABLE)
    set(_RUST_FILTER_PY_CMD "${Python3_EXECUTABLE}")
elseif(WIN32)
    find_program(_PY_LAUNCHER NAMES py)
    if(_PY_LAUNCHER)
        set(_RUST_FILTER_PY_CMD "${_PY_LAUNCHER}" "-3")
    else()
        find_program(_PYTHON_FALLBACK NAMES python3 python)
        if(_PYTHON_FALLBACK)
            set(_RUST_FILTER_PY_CMD "${_PYTHON_FALLBACK}")
        endif()
    endif()
else()
    find_program(_PYTHON_FALLBACK NAMES python3 python)
    if(_PYTHON_FALLBACK)
        set(_RUST_FILTER_PY_CMD "${_PYTHON_FALLBACK}")
    endif()
endif()

if(NOT _RUST_FILTER_PY_CMD)
    message(FATAL_ERROR
        "No Python interpreter found for filter-rust.py. "
        "Install Python or pass -DPython3_EXECUTABLE=<path to python>."
    )
endif()

get_target_property(RUST_ARCHIVE clamav_rust IMPORTED_LOCATION)
set(RUST_FILTERED_ARCHIVE "${RUST_ARCHIVE}.filtered.a")

add_custom_command(
    OUTPUT "${RUST_FILTERED_ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E copy "${RUST_ARCHIVE}" "${RUST_FILTERED_ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E env ${_RUST_FILTER_PY_CMD} "${CMAKE_SOURCE_DIR}/filter-rust.py" "${CMAKE_OBJDUMP}" "${CMAKE_AR}" "${RUST_FILTERED_ARCHIVE}"
    DEPENDS "${RUST_ARCHIVE}" "${CMAKE_SOURCE_DIR}/filter-rust.py"
)

add_custom_target(filter_clamav_rust DEPENDS "${RUST_FILTERED_ARCHIVE}" JOB_POOL single)
set_target_properties(clamav_rust PROPERTIES IMPORTED_LOCATION "${RUST_FILTERED_ARCHIVE}")
add_dependencies(libclamav filter_clamav_rust clamav_rust)
add_dependencies(libfreshclam filter_clamav_rust clamav_rust)
add_dependencies(sigtool filter_clamav_rust clamav_rust)
add_dependencies(clambc filter_clamav_rust clamav_rust)

target_link_options(libclamav PRIVATE $<$<C_COMPILER_ID:MSVC>:/FORCE:MULTIPLE>)
