---
description: How to build and rebuild the ClamWin GUI and ClamAV engine
---

# Build Workflow

## Prerequisites (already installed)
- CMake 4.2.3 — `C:\Program Files\CMake\bin\cmake.exe`
- MinGW-w64 GCC 15.2.0 — `C:\msys64\mingw64\bin\gcc.exe`
- MinGW Make — `C:\msys64\mingw64\bin\mingw32-make.exe`

## PATH Setup
Every build command must have the PATH set first:
```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH
```

## First-Time Configure
```powershell
# // turbo
cmake -S . -B build-gui -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=mingw32-make
```

## Build GUI Only
```powershell
# // turbo
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; Push-Location .\build-gui; mingw32-make clamwin 2>&1 | Select-Object -Last 20; Pop-Location
```

## Reconfigure For Full ClamAV Engine Build
Use this when building scanner/engine targets (`clamscan`, `clamd`, `freshclam`, `sigtool`) in `clamav-win32`.

```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH
$repoRoot = (Get-Location).Path
Remove-Item -Recurse -Force (Join-Path $repoRoot "build-gui")
cmake -S $repoRoot -B (Join-Path $repoRoot "build-gui") -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=mingw32-make -DRUST_COMPILER_TARGET=x86_64-pc-windows-gnu -DCMAKE_C_STANDARD=11 -DCMAKE_C_STANDARD_REQUIRED=ON -DCMAKE_C_EXTENSIONS=ON
rustup target add x86_64-pc-windows-gnu
```

## Build ClamAV Scanner/Engine Targets

Build `clamscan` only:

```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; Push-Location .\build-gui; mingw32-make clamscan 2>&1; Pop-Location
```

Build all ClamAV console tools defined in this workspace configuration:

```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; Push-Location .\build-gui; mingw32-make clamscan freshclam sigtool clamd clamdscan clamdtop 2>&1; Pop-Location
```

## Build Tests
```powershell
# // turbo
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; Push-Location .\build-gui; mingw32-make clamwin_test 2>&1 | Select-Object -Last 20; Pop-Location
```

## Run Tests
```powershell
# // turbo
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; & .\build-gui\clamwin_test.exe
```

## Reconfigure (after CMakeLists.txt changes)
```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; Remove-Item -Recurse -Force .\build-gui; cmake -S . -B build-gui -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=mingw32-make
```

## Important Notes
- Always read the last few lines of terminal output if it fails to see the actual error
- The GUI target is `clamwin`, the test target is `clamwin_test`
- The `clamwin_test` target uses **stubs** for libclamav functions (defined in `stubs_clamav.c`) to avoid linking against the full engine and its Rust dependencies during unit testing.
- Working directory for cmake must be `clamav-win32\`
- Executable output: `build-gui\clamwin.exe`

## Win98 VM Notes
- For Windows 98 VM install/run notes and ISO transfer workflow, see `.agent/workflows/win98-install.md`

## Build and Mount Win98 Drop ISO
Use these commands after copying updated binaries/files into `vdi/W98/iso-staging`.

```powershell
# Build ISO from staging folder (xorriso from MSYS2)
# Note: xorriso expects /c/... style source/output paths.
$repoRoot = (Get-Location).Path
$repoRootMsys = (($repoRoot -replace '\\', '/') -replace '^([A-Za-z]):', '/$1')
& "C:\msys64\usr\bin\xorriso.exe" -as mkisofs -J -R -V WIN98DROP -o "$repoRootMsys/vdi/W98/win98-x86-drop.iso" "$repoRootMsys/vdi/W98/iso-staging"
```

```powershell
# Mount ISO to Win98 VM (VirtualBox)
$repoRoot = (Get-Location).Path
& "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" storageattach W98 --storagectl IDE --port 1 --device 0 --type dvddrive --medium (Join-Path $repoRoot "vdi\W98\win98-x86-drop.iso")
```

## Build GUI + Mount Multi-OS ISO To XP VM (combined)
Builds `clamwin`, syncs binaries into the ISO staging folders, generates the ISO, and mounts it — all in one command. Use this as the standard "build and test on XP" workflow.

```powershell
# // turbo
Stop-Process -Name "clamwin" -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 400; $env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; $repoRoot = (Get-Location).Path; $workspaceRoot = Split-Path $repoRoot -Parent; Push-Location .\build-gui; mingw32-make clamwin 2>&1 | Select-Object -Last 6; Pop-Location; & (Join-Path $workspaceRoot "binaries\sync-iso-from-source.ps1") -SkipBuild -MountVm XPsp3
```

## Remount ISO To XP VM (no rebuild)
Use when you only need to remount the existing ISO (e.g. after VM reboot).

```powershell
$repoRoot = (Get-Location).Path
$workspaceRoot = Split-Path $repoRoot -Parent
& "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" storageattach XPsp3 --storagectl IDE --port 1 --device 0 --type dvddrive --medium (Join-Path $workspaceRoot "binaries\clamwin-all-os.iso")
```
