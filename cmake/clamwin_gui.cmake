# ClamWin GUI — Native Win32 C++ frontend
# Spawns clamscan.exe / freshclam.exe (no libclamav link dependency)

# ── New class-based files (cw_*.cpp) ─────────────────────────
set(clamwin_gui_new
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_main.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_application.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_cli_args.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_dashboard.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_window.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_dialog.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_config.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_process.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_scan_logic.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_prefs_validation.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_scan_dialog.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_about_dialog.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_logview_dialog.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_prefs_dialog.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_schedule_dialog.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_tray.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_scheduler.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_utils.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_theme.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_update_checker.cpp
)

# Legacy function-based GUI modules were removed after the cw_* class migration.
set(clamwin_gui_legacy)

set(clamwin_gui_resources
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/resources/clamwin.rc
)

set(CW_BUILD_COUNTER_FILE ${CMAKE_BINARY_DIR}/cw_gui_build_counter.txt)
set(CW_BUILD_VERSION_HEADER ${CMAKE_BINARY_DIR}/cw_build_version.h)
set(CW_BUILD_VERSION_RC ${CMAKE_BINARY_DIR}/cw_build_version.rc)
set(CW_BUILD_VERSION_SCRIPT ${CLAMWIN_DIR}/cmake/increment_clamwin_build_version.cmake)

# Seed generated version files at configure time so includes exist immediately.
execute_process(
    COMMAND ${CMAKE_COMMAND}
            -DCOUNTER_FILE=${CW_BUILD_COUNTER_FILE}
            -DHEADER_OUT=${CW_BUILD_VERSION_HEADER}
            -DRC_OUT=${CW_BUILD_VERSION_RC}
            -DBASE_MAJOR=${PROJECT_VERSION_MAJOR}
            -DBASE_MINOR=${PROJECT_VERSION_MINOR}
            -DBASE_PATCH=${PROJECT_VERSION_PATCH}
            -DINCREMENT=OFF
            -P ${CW_BUILD_VERSION_SCRIPT}
)

add_custom_target(cw_gui_build_version
    COMMAND ${CMAKE_COMMAND}
            -DCOUNTER_FILE=${CW_BUILD_COUNTER_FILE}
            -DHEADER_OUT=${CW_BUILD_VERSION_HEADER}
            -DRC_OUT=${CW_BUILD_VERSION_RC}
            -DBASE_MAJOR=${PROJECT_VERSION_MAJOR}
            -DBASE_MINOR=${PROJECT_VERSION_MINOR}
            -DBASE_PATCH=${PROJECT_VERSION_PATCH}
            -P ${CW_BUILD_VERSION_SCRIPT}
    BYPRODUCTS ${CW_BUILD_VERSION_HEADER} ${CW_BUILD_VERSION_RC}
    COMMENT "Incrementing ClamWin GUI build number"
    VERBATIM
)

file(GLOB clamwin_gui_headers ${CLAMWIN_DIR}/src/clamwin-gui-cpp/*.h)

source_group("New Classes"   FILES ${clamwin_gui_new})
source_group("Legacy Files"  FILES ${clamwin_gui_legacy})
source_group("Header Files"  FILES ${clamwin_gui_headers})
source_group("Resources"     FILES ${clamwin_gui_resources})

add_executable(clamwin WIN32
    ${clamwin_gui_new}
    ${clamwin_gui_legacy}
    ${clamwin_gui_headers}
    ${clamwin_gui_resources}
)

set(clamwin_gui_test_dir ${CLAMWIN_DIR}/src/clamwin-gui-cpp/tests)
set(clamwin_gui_test_sources
    ${clamwin_gui_test_dir}/test_main.cpp
    ${clamwin_gui_test_dir}/test_support.cpp
    ${clamwin_gui_test_dir}/test_cli_args.cpp
    ${clamwin_gui_test_dir}/test_shell_extension_command.cpp
    ${clamwin_gui_test_dir}/test_config.cpp
    ${clamwin_gui_test_dir}/test_scan_commands.cpp
    ${clamwin_gui_test_dir}/test_scan_parsers.cpp
    ${clamwin_gui_test_dir}/test_scan_transcripts.cpp
    ${clamwin_gui_test_dir}/test_prefs_validation.cpp
    ${clamwin_gui_test_dir}/test_real_tools.cpp
    ${clamwin_gui_test_dir}/test_scheduler.cpp
    ${clamwin_gui_test_dir}/test_utils.cpp
    ${clamwin_gui_test_dir}/test_update_checker.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_config.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_cli_args.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_scan_logic.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_prefs_validation.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_scheduler.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_utils.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/cw_update_checker.cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/shell-extension/cw_shell_extension_command.cpp
)

add_executable(clamwin_gui_test
    ${clamwin_gui_test_sources}
)

add_dependencies(clamwin cw_gui_build_version)
add_dependencies(clamwin_gui_test cw_gui_build_version)

if(MSVC)
    target_sources(clamwin PRIVATE ${CLAMWIN_DIR}/resources/compatibility.manifest)
endif()

target_include_directories(clamwin PRIVATE
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp
    ${CMAKE_BINARY_DIR}
)

target_include_directories(clamwin_gui_test PRIVATE
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp
    ${CLAMWIN_DIR}/src/clamwin-gui-cpp/shell-extension
    ${CLAMWIN_DIR}/3rdparty/doctest
    ${clamwin_gui_test_dir}
    ${CMAKE_BINARY_DIR}
)

target_compile_definitions(clamwin PRIVATE
    ${CLAMWIN_DEFINES}
    _CRT_SECURE_NO_WARNINGS
)

target_compile_definitions(clamwin_gui_test PRIVATE
    ${CLAMWIN_DEFINES}
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(clamwin PRIVATE
    comctl32
    shlwapi
    comdlg32
    shell32
    ole32
    oleaut32
    gdi32
    user32
    kernel32
    advapi32 gdiplus
    version
    wininet
)

target_link_libraries(clamwin_gui_test PRIVATE
    comctl32
    shlwapi
    comdlg32
    shell32
    ole32
    gdi32
    user32
    kernel32
    advapi32 gdiplus
    version
    wininet
)

add_custom_target(clamwin_gui_check
    COMMAND $<TARGET_FILE:clamwin_gui_test>
    DEPENDS clamwin_gui_test
    WORKING_DIRECTORY $<TARGET_FILE_DIR:clamwin_gui_test>
    USES_TERMINAL
)

add_custom_target(clamwin_gui_check_real_tools
    COMMAND ${CMAKE_COMMAND} -E env CLAMWIN_REAL_TOOLS=1 $<TARGET_FILE:clamwin_gui_test>
    DEPENDS clamwin_gui_test
    WORKING_DIRECTORY $<TARGET_FILE_DIR:clamwin_gui_test>
    USES_TERMINAL
)

add_subdirectory(${CLAMWIN_DIR}/src/clamwin-gui-cpp/shell-extension)

list(APPEND CLAMAV_INSTALL_TARGETS clamwin)
