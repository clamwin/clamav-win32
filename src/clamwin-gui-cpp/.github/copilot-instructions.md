# Copilot Instructions — ClamWin Uplift Project

## Project Overview

This workspace converts the legacy **ClamWin** antivirus GUI from Python/wxPython to a **native C++ Win32 application** with a Google Drive-inspired visual design. The new GUI lives in `clamav-win32/src/clamwin-gui-cpp/` and builds with CMake + MinGW.

The application does **not** link against libclamav. It spawns `clamscan.exe` and `freshclam.exe` as child processes and captures their output.

## Architecture

### Class Hierarchy

| Class | Role | Base |
|---|---|---|
| `CWApplication` | Singleton. Owns config, tray, scheduler, dashboard. Entry point via `run()`. | — |
| `CWWindow` | Base for top-level windows. Static `WndProc` → `GWLP_USERDATA` → virtual dispatch. | — |
| `CWDialog` | Base for modal dialogs created from in-memory `DLGTEMPLATE`. | — |
| `CWDashboard` | Main card-based dashboard. | `CWWindow` |
| `CWScanDialog` | Scan/update progress with RichEdit log. | `CWDialog` |
| `CWPrefsDialog` | Preferences (stub — needs full implementation). | `CWDialog` |
| `CWScheduleDialog` | Schedule settings (stub — needs full implementation). | `CWDialog` |
| `CWAboutDialog` | Version/credits display. | `CWDialog` |
| `CWLogViewDialog` | Log file viewer. | `CWDialog` |
| `CWProcess` | RAII wrapper for `CreateProcess` + pipe capture. | — |
| `CWConfig` | INI-based config via `GetPrivateProfileString`. | — |
| `CWTray` | System tray icon management. | — |
| `CWScheduler` | Timer-based job triggering. | — |
| `CWTheme` | Light/Dark palette with cached GDI brushes. | — |

### Key Files

- Entry point: `cw_main.cpp` → `CWApplication::run()`
- CMake target: `cmake/clamwin_gui.cmake` (target name: `clamwin`)
- Shared defs: `cw_gui_shared.h` (resource IDs, message constants, shared GUI types)
- Platform defs: `cwdefs.h` (`_WIN32_WINNT 0x0410`)
- DPI helper: `cw_dpi.h` (`CW_Scale()`)
- Theme: `cw_theme.h`/`.cpp` (Google Drive palette, dark/light detection)

## Build System

### Toolchain
- **CMake 4.x** at `C:\Program Files\CMake\bin\cmake.exe`
- **MinGW-w64 GCC 15.x** at `C:\msys64\mingw64\bin\gcc.exe`
- **Make** via `mingw32-make`

### Build Commands

Always set PATH first:
```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH
```

Configure (first time or after CMakeLists.txt changes):
```powershell
cmake -S . -B build-gui -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=mingw32-make
```

Build GUI:
```powershell
Push-Location .\build-gui; mingw32-make clamwin 2>&1 | Select-Object -Last 20; Pop-Location
```

Build tests:
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\test-gui.ps1
```

Run real-tool smoke tests:
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\test-gui.ps1 -RealTools
```

Run stricter gated FreshClam probes:
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\test-gui.ps1 -RealTools -FreshclamUpdate -FreshclamNegative
```

### Stopping the running instance
The linker cannot overwrite `clamwin.exe` while it is running. Always kill it before rebuilding:
```powershell
Stop-Process -Name "clamwin" -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 400
```
Combine with a build in one line:
```powershell
Stop-Process -Name "clamwin" -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 400; Push-Location .\build-gui; mingw32-make clamwin 2>&1 | Select-Object -Last 10; Pop-Location
```

### Working Directory
All cmake/make commands run from `clamav-win32/`.

## Coding Standards

### Naming
- **Classes**: `CWPascalCase` (e.g. `CWDashboard`, `CWScanDialog`)
- **Methods**: `camelCase` (e.g. `onCreate`, `onPaint`)
- **Virtual handlers**: `onVerb` (e.g. `onCommand`, `onDestroy`)
- **Members**: `m_camelCase` (e.g. `m_hwnd`, `m_config`)
- **Static/global**: `s_prefixed` (e.g. `s_instance`)
- **Constants/IDs**: `UPPER_CASE` with prefix (`IDC_`, `IDM_`, `CW_`)
- **Files**: `cw_snake_case.cpp/h`

### Style
- 4-space indentation, no tabs
- Allman brace style (opening brace on its own line)
- `#pragma once` for header guards
- Forward-declare instead of including where possible
- One class per `.cpp`/`.h` pair

### C++ Features — SAFE (compile-time, statically linked)
- Classes, virtual functions, inheritance, RAII
- `std::string`, `std::vector` (via static libstdc++)
- `enum class`, references, `const`
- Constructors/destructors for resource cleanup

### C++ Features — AVOID
- `std::thread`, `std::mutex` → use `CreateThread`, `CRITICAL_SECTION`
- `std::filesystem` → uses Win32 APIs not on Win98
- C++ exceptions → use return codes
- `std::unique_ptr` → manual RAII in destructors
- Unicode `W` APIs → ANSI `A` APIs only

### Win32 Constraints
- `_WIN32_WINNT 0x0410` (Windows 98 minimum)
- ANSI APIs only (no `W` suffix functions)
- `GWL_USERDATA` not `GWLP_USERDATA` on true Win9x
- `InitCommonControls()` for Win95/98 compatibility
- Static CRT linking (`-static`)
- All dimensions through `CW_Scale()` for DPI awareness

### Error Handling
- No exceptions — return codes and booleans
- No raw `new`/`delete` outside singletons — stack allocation or `std::vector`

## Design Language — Google Drive Style

### Theme System (`CWTheme`)
The theme system detects Windows light/dark mode via registry (`AppsUseLightTheme`) and provides:

| Token | Light | Dark |
|---|---|---|
| Background | `#f0f4f9` | `#1f1f1f` |
| Surface (cards) | `#ffffff` | `#2d2d2d` |
| Surface Hover | `#f5f7fb` | `#3c3c3c` |
| Text | `#1f1f1f` | `#e3e3e3` |
| Text Muted | `#5a5a5a` | `#a0a0a0` |
| Accent | `#3f8dfd` (Google Blue) | same |
| Success | `#0f9d58` (Google Green) | same |
| Warning | `#ea4335` (Google Red) | same |

### Visual Principles
- **Rounded rectangles** (`RoundRect` API) for cards and group boxes
- **No 3D bevels** — flat look with subtle shadows via slightly darker borders
- **Typography**: "Segoe UI" primary, fallback "Tahoma" → "MS Sans Serif"
- **Owner-drawn controls** for buttons, checkboxes, sidebar items to consume theme colors
- **Double-buffered painting** to prevent flicker
- **Hover states** on interactive elements (slightly lighter surface color)
- **Sidebar navigation** in preferences (left pane = nav, right pane = content)

## Legacy Reference

The original Python GUI is at `clamwin/py/`. When implementing new features:
1. **Always cross-reference** the equivalent Python class for functional parity
2. Key mappings:
   - `wxDialogPreferences.py` → `CWPrefsDialog`
   - `wxDialogScheduledScan.py` → `CWScheduleDialog`
   - `wxDialogStatus.py` → `CWScanDialog`
   - `wxFrameMain.py` → `CWDashboard` + `CWApplication`
   - `Config.py` → `CWConfig`
   - `Process.py` → `CWProcess`
   - `Scheduler.py` → `CWScheduler`

## Current Project Status (Phase 4)

### Completed
- Full C++ class architecture (CWApplication, CWWindow, CWDialog hierarchy)
- Dashboard with card layout and status banner
- Scan/Update dialog with process management and RichEdit log
- Config load/save (INI-based, ~30 fields)
- Tray icon with context menu
- Scheduler (timer-based)
- About and LogView dialogs
- CWTheme class (dark/light detection + palette)

### In Progress — Phase 4: Google Drive Aesthetic Polish
- Wire CWTheme into all windows and dialogs (currently not consumed)
- Implement `CWPrefsDialog` with sidebar navigation and all config tabs
- Implement `CWScheduleDialog` with full scheduling controls
- Create owner-drawn custom controls (buttons, checkboxes, sidebar items)
- Add tray state icons (OK/Warning/Error) to resource file

## Testing

- Test framework: doctest (vendored in `clamav-win32/3rdparty/doctest/`)
- Test executable: `clamwin_gui_test.exe`
- Tests go in `src/clamwin-gui-cpp/tests/`
- Default test command: `powershell -ExecutionPolicy Bypass -File .\tools\test-gui.ps1`
- Real-tool smoke command: `powershell -ExecutionPolicy Bypass -File .\tools\test-gui.ps1 -RealTools`
- Build-only target: `mingw32-make clamwin_gui_test`

## Important Reminders

1. **Build and verify** after every significant change
2. **No dynamic dependencies** — single static executable
3. **Config backward-compatible** with original `ClamWin.conf` format
4. **DPI-scale everything** — use `CW_Scale()` for all fixed dimensions
5. **RAII** — destructors must clean up all GDI objects, handles, threads
6. **Cross-reference legacy Python** for exact feature parity
