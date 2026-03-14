---
description: Windows 98 VM install and ISO transfer workflow for x86 ClamWin/ClamAV builds
---

# Win98 Install Workflow

## VM Image Reference
- Archive reference: https://archive.org/details/win98vb

## Build Outputs To Package
Expected x86 outputs from `clamav-win32/build-x86-mingw-win98/`:
- `clamwin.exe`
- `clamscan.exe`
- `freshclam.exe`
- `sigtool.exe`
- `clamd.exe`
- `clamdscan.exe`
- `clamdtop.exe`
- `libclamav.dll`
- `libfreshclam.dll`

## Create ISO Staging Folder
Run from host PowerShell:

```powershell
$root = (Get-Location).Path
$build = Join-Path $root "build-x86-mingw-win98"
$stage = Join-Path $root "vdi\W98\iso-staging"

Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$stage\bin","$stage\db","$stage\logs" | Out-Null

Copy-Item "$build\clamwin.exe","$build\clamscan.exe","$build\freshclam.exe","$build\sigtool.exe","$build\clamd.exe","$build\clamdscan.exe","$build\clamdtop.exe","$build\libclamav.dll","$build\libfreshclam.dll" "$stage\bin" -Force
Copy-Item "$root\freshclam.conf","$root\clamd.conf","$root\clamav.reg" "$stage" -Force
Copy-Item "$root\clam.hdb" "$stage\db" -Force
```

## Build ISO
Requires MSYS2 `xorriso` (`pacman -S --needed xorriso`).

```powershell
$root = (Get-Location).Path
$rootMsys = (($root -replace '\\', '/') -replace '^([A-Za-z]):', '/$1')
C:\msys64\usr\bin\bash.exe -lc "xorriso -as mkisofs -J -R -V CLAMWIN98 -o '$rootMsys/vdi/W98/win98-x86-drop.iso' '$rootMsys/vdi/W98/iso-staging'"
```

Output ISO:
- `clamav-win32/vdi/W98/win98-x86-drop.iso`

## Attach ISO To VM

```powershell
$root = (Get-Location).Path
& "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" storageattach W98 --storagectl IDE --port 1 --device 0 --type dvddrive --medium (Join-Path $root "vdi\W98\win98-x86-drop.iso")
```

Start VM:

```powershell
& "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" startvm W98 --type gui
```

## In-Guest Runtime Checks
From Win98 command prompt after copying files locally from the mounted CD:

```bat
clamscan.exe --version
freshclam.exe --version
sigtool.exe --version
clamd.exe --version
clamdscan.exe --version
clamdtop.exe --version
clamwin.exe
```

Capture output to a log file when validating compatibility.
