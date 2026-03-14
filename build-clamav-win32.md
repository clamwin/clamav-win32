# Build ClamAV Win32 (x64)

This project builds reliably on this machine with MinGW + CMake + Rust when using a dedicated x64 build directory and explicit toolchain settings.

## One-Time Prerequisites

```powershell
$env:PATH = "C:\Users\alexc\.cargo\bin;C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH
rustup target add x86_64-pc-windows-gnu
```

## Configure + Build (x64)

Run from `clamav-win32/`:

```powershell
$env:PATH = "C:\Users\alexc\.cargo\bin;C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH

cmake -S . -B build-x64-mingw -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM=mingw32-make `
  -DCMAKE_C_COMPILER=gcc `
  -DCMAKE_CXX_COMPILER=g++ `
  -DCMAKE_RC_COMPILER=windres `
  -DCLAMWIN_SHELLEXT_UNICODE=ON `
  -DCMAKE_TOOLCHAIN_FILE=c:/Users/alexc/source/repos/clamwin-uplift/clamav-win32/cmake/toolchain-mingw-x64.cmake `
  -DCMAKE_C_FLAGS="-std=gnu17" `
  -DRUST_COMPILER_TARGET:STRING=x86_64-pc-windows-gnu

cmake --build build-x64-mingw -j 4
```

## Why These Flags

- `-DCMAKE_TOOLCHAIN_FILE=...toolchain-mingw-x64.cmake`: forces x64 MinGW target configuration.
- `-DCMAKE_RC_COMPILER=windres`: uses the RC tool that exists in this environment.
- `-DCMAKE_C_FLAGS="-std=gnu17"`: avoids GCC 15 keyword collision with legacy `alignof` symbol in ClamAV C sources, without changing source code.
- `-DRUST_COMPILER_TARGET:STRING=x86_64-pc-windows-gnu`: keeps Rust static lib output compatible with MinGW build artifacts.
- `-DCLAMWIN_SHELLEXT_UNICODE=ON`: explicitly locks shell extension to Unicode mode for x64/xp class builds.
- `rustup target add x86_64-pc-windows-gnu`: installs the Rust stdlib required for the GNU target.

## Output Location

Main binaries and DLLs are emitted under:

- `build-x64-mingw/`

Examples:

- `clamscan.exe`
- `freshclam.exe`
- `clamd.exe`
- `clamdscan.exe`
- `clamdtop.exe`
- `sigtool.exe`
- `libclamav.dll`
- `libfreshclam.dll`

## Smoke Test

Run from `build-x64-mingw/`:

```powershell
$root = "C:\Users\alexc\source\repos\clamwin-uplift\clamav-win32"

.\clamscan.exe --version
.\sigtool.exe --version
.\freshclam.exe --config-file "$root\freshclam.conf" --version
.\clamdscan.exe --config-file "$root\clamd.conf" --version
.\clamd.exe --config-file "$root\clamd.conf" --version
```

Expected result: each command prints `ClamAV 1.5.2` (or the currently built version) and exits successfully.

## GUI Test Suite

The native Win32 C++ GUI now has a dedicated test executable:

- `clamwin_gui_test.exe`

For normal day-to-day use, prefer the wrapper script from the `clamav-win32/` root:

```powershell
.\test-gui.ps1
```

Useful variants:

```powershell
.\test-gui.ps1 -RealTools
.\test-gui.ps1 -RealTools -FreshclamUpdate -FreshclamNegative
.\test-gui.ps1 -BuildOnly
```

Build it from `clamav-win32/` with the same MinGW + CMake environment used for the GUI build:

```powershell
$env:PATH = "C:\Users\alexc\.cargo\bin;C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH

cmake -S . -B build-gui -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM=mingw32-make `
  -DCMAKE_C_COMPILER=gcc `
  -DCMAKE_CXX_COMPILER=g++

cmake --build build-gui --target clamwin_gui_test
```

Run the default GUI tests directly:

```powershell
Set-Location build-gui
.\clamwin_gui_test.exe
```

Or run them through the dedicated make target:

```powershell
cmake --build build-gui --target clamwin_gui_check
```

The wrapper script above calls the same targets and sets the required environment variables for the optional gated tests.

### What The Default GUI Suite Covers

- `CWConfig` save/load round trips and default behavior
- clamscan / freshclam command construction
- stdout parsing helpers for clamscan and freshclam
- transcript-driven integration tests using fixture files under `src/clamwin-gui-cpp/tests/fixtures`

### Real Tool Smoke Tests

The GUI suite also contains opt-in smoke tests that execute the built `clamscan.exe` and `freshclam.exe` from the build directory.

Run them by enabling `CLAMWIN_REAL_TOOLS=1`:

```powershell
.\test-gui.ps1 -RealTools
```

The real-tool smoke path expects these files in the test executable directory, which is true for the normal `build-gui` layout:

- `clamscan.exe`
- `freshclam.exe`
- `freshclam.conf`
- `db\`

### Stricter FreshClam Update Probe

There is a stricter FreshClam integration path that attempts a real parseable update/output probe. It is intentionally not part of the default smoke target because it may depend on live network and mirror behavior.

Enable it only when you want that extra validation:

```powershell
.\test-gui.ps1 -RealTools -FreshclamUpdate
```

### FreshClam Negative Smoke Test

There is also an opt-in negative real-tool probe that runs `freshclam.exe` with a deliberately invalid config file and verifies that the emitted stderr remains parseable and does not masquerade as update progress.

Enable it only when you want to validate error-path behavior from the real binary:

```powershell
.\test-gui.ps1 -RealTools -FreshclamNegative
```

If you need to override binary or database paths, these environment variables are supported:

- `CLAMWIN_CLAMSCAN_EXE`
- `CLAMWIN_FRESHCLAM_EXE`
- `CLAMWIN_DB_DIR`
