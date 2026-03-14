---
description: ClamWin GUI coding standards and project conventions
---

# ClamWin GUI Coding Standards

## Project Overview
This project is a **native C++ class-based Win32 API rewrite** of the ClamWin GUI. The old GUI was Python/wxPython. The original C translation was further refactored into a clean C++ Object-Oriented architecture. The new GUI lives in `clamav-win32/src/clamwin-gui-cpp/` and integrates into the clamav-win32 CMake build system.

## Key Constraints
- **OS Compatibility**: Must compile and run on Windows 98 (with `WINVER 0x0410` masks) through Windows 11.
- **Toolchain**: MinGW-w64 GCC (Static libstdc++), CMake.
- **Language**: Standard C++. STL structures like `std::string` and `std::vector` are permitted via static CRT linking, enabling seamless Win9x operation without dependencies.
- **No external UI libs**: Win32 API natively (comctl32, shell32, gdi32, etc.) mapped through C++ encapsulation.

## Source Layout
```
clamav-win32/
├── src/clamwin-gui-cpp/
│   ├── cwdefs.h               # Platform headers & Windows Versioning
│   ├── clamwin_gui.h          # Shared project constants / IDs
│   ├── cw_main.cpp            # WinMain entry point
│   ├── cw_application.cpp     # Singleton application lifecycle & tray
│   ├── cw_window.cpp          # Base generic Win32 window class
│   ├── cw_dialog.cpp          # Base dialog wrapper class
│   ├── cw_dashboard.cpp       # Main dashboard layout
│   ├── cw_scan_dialog.cpp     # Scanner and updater UI flow
│   ├── cw_config.cpp          # Config (INI) reader/writer
│   ├── cw_process.cpp         # Subprocess piping/management (RAII)
│   ├── cw_tray.cpp            # Notification area icon lifecycle
│   ├── cw_scheduler.cpp       # Time-triggered jobs
│   ├── cw_utils.cpp           # Helpers / DB queries
│   ├── cw_prefs_dialog.cpp    # Preferences tabs layout
│   ├── cw_schedule_dialog.cpp # Schedule configuration layouts
│   ├── cw_about_dialog.cpp    # App about box display
│   ├── cw_logview_dialog.cpp  # Dialog for reading log outputs
│   ├── resources/
│   │   ├── clamwin.rc         # Resource script
│   │   ├── clamwin.manifest   # App manifest (visual styles, DPI, UAC)
│   │   └── clamwin.ico        # Main icon
├── cmake/
│   └── clamwin_gui.cmake      # C++ UI CMake rules
```

## Naming Conventions
- **Classes**: `CWClassName` (e.g. `CWConfig`, `CWDashboard`, `CWScanDialog`).
- **Methods**: `camelCase` (e.g. `onInit()`, `runModal()`).
- **Class Members**: `m_` prefix, `camelCase` (e.g. `m_hwnd`, `m_config`).
- **Constants**: `CW_` prefix, `UPPER_SNAKE` (e.g. `CW_MAX_PATH`).
- **Resource IDs**: `IDI_`, `IDC_`, `IDM_` prefixes, `UPPER_SNAKE`.

## Code Style
- 4-space indentation (no tabs)
- Allman brace style (opening brace on new line)
- Include appropriate headers. Base Win32 logic should inherit from either `CWWindow` (dynamically created) or `CWDialog` (modal resources).
- Class Encapsulation: Do not use global state where a class instance (like `CWApplication`) covers the lifecycle.
- Resource cleanup must use RAII (e.g. `CWProcess` dtors terminating handles).

## Win32 API Patterns
- **High DPI**: All dialog dimensions and fonts must be run through `CW_Scale()` inline functions to ensure 4K monitors correctly handle fixed geometries. 
- **Legacy fallback**: Fallbacks must remain functional on NT 4.0 / 9x. Keep API calls limited to those generally present within the `0x0410` Win32 Headers.
- **Double-buffering**: Custom-painted windows (like CWDashboard) use off-screen bitmaps and subclassed drawing contexts (`WM_PAINT` & `WM_ERASEBKGND` management) to prevent flicker.

## Config Compatibility
The config file format is tightly backward-compatible with the original Python `ClamWin.conf`. Our `CWConfig` class maps legacy file formatting to internal properties.

## Critical Rules
1. **Always refer to the legacy wxPython codebase** (`clamwin/py/`) when implementing features for the new GUI to maintain exact functional parity. 
2. **Build and verify** after every significant change using the `/build` workflow. Address both syntax and linker outputs cleanly across components.
3. **No Dynamic Dependencies**: Because we target legacy deployment, all code should compile into a singular light Executable.
4. **Adhere to the OOP Architecture**: When migrating legacy logic, wrap resources cleanly with constructor and destructor behaviors.
