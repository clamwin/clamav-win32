param(
    [Alias('SkipClamBuild','SkipClamAvBuild')]
    [switch]$SkipBuild,
    [switch]$GenerateIso,
    [string]$DatabaseSource = "",
    [string[]]$MountVm = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Convert-ToMsysPath {
    param([Parameter(Mandatory = $true)][string]$WindowsPath)

    $full = [System.IO.Path]::GetFullPath($WindowsPath)
    $drive = $full.Substring(0, 1).ToLowerInvariant()
    $rest = $full.Substring(2).Replace('\', '/')
    if (-not $rest.StartsWith('/')) {
        $rest = '/' + $rest
    }
    return "/$drive$rest"
}

function Get-ExpectedRustTarget {
    param([string[]]$ConfigureArgs)

    foreach ($arg in $ConfigureArgs) {
        if ($arg -match '^-DRUST_COMPILER_TARGET(?::STRING)?=(.+)$') {
            return $Matches[1]
        }
    }

    return ""
}

function Get-ProjectVersion {
    param([Parameter(Mandatory = $true)][string]$CMakeListsPath)

    if (-not (Test-Path $CMakeListsPath)) {
        throw "CMakeLists.txt not found: $CMakeListsPath"
    }

    $hit = Select-String -Path $CMakeListsPath -Pattern '^\s*project\s*\(\s*clamav-win32\s+VERSION\s+"([0-9]+\.[0-9]+\.[0-9]+)"\s*\)' -CaseSensitive:$false | Select-Object -First 1
    if (-not $hit) {
        throw "Unable to detect clamav-win32 version from: $CMakeListsPath"
    }

    return $hit.Matches[0].Groups[1].Value
}

function Resolve-StagePackageBySuffix {
    param(
        [Parameter(Mandatory = $true)][string]$StageRoot,
        [Parameter(Mandatory = $true)][string]$Suffix,
        [string]$PreferredName = ""
    )

    if (-not (Test-Path $StageRoot)) {
        throw "Stage root not found: $StageRoot"
    }

    if (-not [string]::IsNullOrWhiteSpace($PreferredName)) {
        $preferredPath = Join-Path $StageRoot $PreferredName
        if (Test-Path $preferredPath) {
            return $PreferredName
        }
    }

    $candidates = Get-ChildItem -Path $StageRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like "clamav-*-$Suffix" }

    if (-not $candidates -or $candidates.Count -eq 0) {
        throw "No stage package found for suffix '$Suffix' under '$StageRoot'"
    }

    return ($candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).Name
}

function Invoke-BuildTarget {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string[]]$Targets,
        [Parameter(Mandatory = $true)][bool]$UseMingw32,
        [string]$RustTarget = ""
    )

    $mingwPath = if ($UseMingw32) { "C:\msys64\mingw32\bin" } else { "C:\msys64\mingw64\bin" }
    $env:PATH = "$mingwPath;C:\Program Files\CMake\bin;" + $env:PATH
    # Enforce GNU11 for all C compilation units, including libclamav, without source edits.
    $env:CFLAGS = "-std=gnu11"
    if (-not [string]::IsNullOrWhiteSpace($RustTarget)) {
        $env:CARGO_BUILD_TARGET = $RustTarget
    }

    $targetText = [string]::Join(' ', $Targets)
    $cmd = "cd /d `"$BuildDir`" && mingw32-make $targetText 2>&1"

    Write-Host "[build] $BuildDir => $targetText"
    & cmd.exe /c $cmd
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed in $BuildDir (exit $LASTEXITCODE)"
    }
}

function Get-RustArchiveDependencyFromBuildMake {
    param([Parameter(Mandatory = $true)][string]$BuildDir)

    $buildMake = Join-Path $BuildDir "CMakeFiles\libclamav.dir\build.make"
    if (-not (Test-Path $buildMake)) {
        return ""
    }

    $hit = Select-String -Path $buildMake -Pattern 'libclamav_rust\.a' -CaseSensitive:$false | Select-Object -First 1
    if (-not $hit) {
        return ""
    }

    $line = $hit.Line
    if ($line -match '([A-Za-z0-9_./-]*libclamav_rust\.a)') {
        return $Matches[1]
    }

    return ""
}

function Invoke-ConfigureProfile {
    param([Parameter(Mandatory = $true)][hashtable]$Profile)

    if (-not $Profile.ContainsKey("ConfigureArgs")) {
        return
    }

    $mingwPath = if ($Profile.UseMingw32) { "C:\msys64\mingw32\bin" } else { "C:\msys64\mingw64\bin" }
    $env:PATH = "$mingwPath;C:\Program Files\CMake\bin;" + $env:PATH
    # Keep CMake-generated build rules pinned to GNU11 even if cache is regenerated.
    $env:CFLAGS = "-std=gnu11"

    $expectedRustTarget = Get-ExpectedRustTarget -ConfigureArgs $Profile.ConfigureArgs
    $cacheFile = Join-Path $Profile.BuildDir "CMakeCache.txt"
    if ($expectedRustTarget -and (Test-Path $cacheFile)) {
        $cachedRustTarget = ""
        $cacheHit = Select-String -Path $cacheFile -Pattern '^RUST_COMPILER_TARGET(?::[A-Z]+)?=(.+)$' -CaseSensitive:$false | Select-Object -First 1
        if ($cacheHit) {
            $cachedRustTarget = $cacheHit.Matches[0].Groups[1].Value.Trim()
        }

        if ($cachedRustTarget -and ($cachedRustTarget -ne $expectedRustTarget)) {
            Write-Host ("[configure] clearing stale cache for {0}: rust target {1} -> {2}" -f $Profile.Name, $cachedRustTarget, $expectedRustTarget)
            if (Test-Path $cacheFile) {
                Remove-Item $cacheFile -Force
            }

            $cacheDir = Join-Path $Profile.BuildDir "CMakeFiles"
            if (Test-Path $cacheDir) {
                Remove-Item $cacheDir -Recurse -Force
            }
        }
    }

    if ($expectedRustTarget) {
        $env:CARGO_BUILD_TARGET = $expectedRustTarget
    }

    $invokeConfigure = {
        $args = @()
        # Force a stable C dialect for all profiles to avoid compiler-default keyword collisions.
        $args += @("-DCMAKE_C_STANDARD=11", "-DCMAKE_C_STANDARD_REQUIRED=ON", "-DCMAKE_C_EXTENSIONS=ON")
        $shellExtUnicode = if ($Profile.Name -eq "win9x") { "OFF" } else { "ON" }
        $args += @("-DCLAMWIN_SHELLEXT_UNICODE=$shellExtUnicode")
        if ($expectedRustTarget) {
            # Pass both typed and untyped forms to avoid option typing edge-cases in subprojects.
            $args += @("-DRUST_COMPILER_TARGET=$expectedRustTarget")
        }
        $args += $Profile.ConfigureArgs
        & cmake @args
        if ($LASTEXITCODE -ne 0) {
            throw "Configure failed for $($Profile.Name) (exit $LASTEXITCODE)"
        }
    }

    Write-Host "[configure] $($Profile.BuildDir)"
    & $invokeConfigure

    if ($expectedRustTarget) {
        $expectedRustArchive = "$expectedRustTarget/release/libclamav_rust.a"
        $actualRustArchive = Get-RustArchiveDependencyFromBuildMake -BuildDir $Profile.BuildDir
        if ($actualRustArchive -and ($actualRustArchive -ne $expectedRustArchive)) {
            Write-Host ("[configure] forcing clean regenerate for {0}: rust archive {1} -> {2}" -f $Profile.Name, $actualRustArchive, $expectedRustArchive)

            if (Test-Path $cacheFile) {
                Remove-Item $cacheFile -Force
            }

            $cacheDir = Join-Path $Profile.BuildDir "CMakeFiles"
            if (Test-Path $cacheDir) {
                Remove-Item $cacheDir -Recurse -Force
            }

            & $invokeConfigure

            $actualRustArchive = Get-RustArchiveDependencyFromBuildMake -BuildDir $Profile.BuildDir
            if ($actualRustArchive -and ($actualRustArchive -ne $expectedRustArchive)) {
                throw "Rust target mismatch after reconfigure for $($Profile.Name): expected '$expectedRustArchive', got '$actualRustArchive'"
            }
        }
    }
}

function Get-LatestSetupNodbExe {
    param([Parameter(Mandatory = $true)][string]$SetupOutputDir)

    if (-not (Test-Path $SetupOutputDir)) {
        return ""
    }

    $latest = Get-ChildItem -Path $SetupOutputDir -File |
        Where-Object { $_.Name -match '(^clamwin-.*-setup-nodb\.exe$)|(^Setup-nodb\.exe$)|(^Setup\.exe$)' } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $latest) {
        return ""
    }

    return $latest.FullName
}

function Invoke-BuildSetupNodb {
    param(
        [Parameter(Mandatory = $true)][string]$ISToolExe,
        [Parameter(Mandatory = $true)][string]$SetupDir,
        [Parameter(Mandatory = $true)][string]$SetupScript,
        [Parameter(Mandatory = $true)][string]$SetupOutputDir
    )

    if (-not (Test-Path $ISToolExe)) {
        throw "ISTool not found: $ISToolExe"
    }
    if (-not (Test-Path $SetupScript)) {
        throw "Setup script not found: $SetupScript"
    }

    Write-Host "[setup] building $SetupScript via ISTool (-compile)"
    Push-Location $SetupDir
    try {
        $proc = Start-Process -FilePath $ISToolExe -ArgumentList @('-compile', $SetupScript) -WorkingDirectory $SetupDir -Wait -PassThru
        if ($proc.ExitCode -ne 0) {
            throw "ISTool compile failed (exit $($proc.ExitCode))"
        }
    }
    finally {
        Pop-Location
    }

    $setupExe = Get-LatestSetupNodbExe -SetupOutputDir $SetupOutputDir
    if ([string]::IsNullOrWhiteSpace($setupExe) -or -not (Test-Path $setupExe)) {
        throw "Setup-nodb output not found in: $SetupOutputDir"
    }

    Write-Host "[setup] built $setupExe"
    return $setupExe
}

$clamavRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\..\.."))
$repoRoot = Split-Path $clamavRoot -Parent
$stageRoot = Join-Path $repoRoot "binaries\all-os-staging"
$isoPath = Join-Path $repoRoot "binaries\clamwin-all-os.iso"
$projectVersion = Get-ProjectVersion -CMakeListsPath (Join-Path $clamavRoot "CMakeLists.txt")
$stagePackagePrefix = "clamav-$projectVersion"
$isToolExe = "C:\Program Files (x86)\ISTool\ISTool.exe"
$setupDir = Join-Path $clamavRoot "src\clamwin-gui-cpp\setup"
$setupNodbIss = Join-Path $setupDir "Setup-nodb.iss"
$setupOutputDir = Join-Path $setupDir "Output"
$bashExe = "C:\msys64\usr\bin\bash.exe"
$certSrc = Join-Path $clamavRoot "clamav\certs\clamav.crt"
$shellExtRegTemplate = Join-Path $clamavRoot "src\clamwin-gui-cpp\shell-extension\cw_shell_extension.reg"
$xpRefreshBatTemplate = Join-Path $clamavRoot "src\clamwin-gui-cpp\tools\xp-refresh-clamav-bin.bat"
$defaultDbSource = Join-Path $clamavRoot "build-gui\db"
$dbFallbackSingle = Join-Path $clamavRoot "clam.hdb"

if (!(Test-Path $stageRoot)) {
    throw "Staging folder not found: $stageRoot"
}
if (!(Test-Path $bashExe)) {
    throw "MSYS bash not found: $bashExe"
}
if (!(Test-Path $certSrc)) {
    throw "Certificate source not found: $certSrc"
}
if (!(Test-Path $shellExtRegTemplate)) {
    throw "Shell extension reg template not found: $shellExtRegTemplate"
}
if (!(Test-Path $xpRefreshBatTemplate)) {
    throw "XP refresh batch template not found: $xpRefreshBatTemplate"
}

if ([string]::IsNullOrWhiteSpace($DatabaseSource)) {
    $DatabaseSource = $defaultDbSource
}

if ($SkipBuild) {
    Write-Host "[build] SkipBuild enabled: skipping configure/compile; installer will still be rebuilt"
}

$dbFileList = @()
if (Test-Path $DatabaseSource) {
    $dbFileList = @(Get-ChildItem -Path $DatabaseSource -File -Include *.cvd,*.cld,*.cud,*.hdb,*.ndb,*.ldb,*.ign2 -ErrorAction SilentlyContinue)
}

if ($dbFileList.Count -eq 0 -and (Test-Path $dbFallbackSingle)) {
    $dbFileList = @(Get-Item $dbFallbackSingle)
}

if ($dbFileList.Count -eq 0) {
    throw "No virus database files found. Checked '$DatabaseSource' and fallback '$dbFallbackSingle'."
}

Write-Host ("[db] using source '{0}' with {1} file(s)" -f $DatabaseSource, $dbFileList.Count)
Write-Host ("[version] detected clamav-win32 version: {0}" -f $projectVersion)

$legacyWin9xStagePackage = Resolve-StagePackageBySuffix -StageRoot $stageRoot -Suffix "legacy-win9x" -PreferredName "$stagePackagePrefix-legacy-win9x"
Write-Host ("[stage] win9x package: {0}" -f $legacyWin9xStagePackage)

$profiles = @(
    @{
        Name = "win9x"
        StagePackage = $legacyWin9xStagePackage
        BuildDir = Join-Path $clamavRoot "build-x86-mingw-win98"
        UseMingw32 = $true
        ConfigureArgs = @(
            "-S", $clamavRoot,
            "-B", (Join-Path $clamavRoot "build-x86-mingw-win98"),
            "-G", "MinGW Makefiles",
            "-DCMAKE_MAKE_PROGRAM=mingw32-make",
            "-DCMAKE_C_COMPILER=gcc",
            "-DCMAKE_CXX_COMPILER=g++",
            "-DCMAKE_RC_COMPILER=windres",
            "-DCMAKE_C_FLAGS=-std=gnu11 -Wno-error=implicit-function-declaration",
            "-DENABLE_LEGACY=ON",
            "-DCLAMWIN_SHELLEXT_UNICODE=OFF",
            "-DRUST_COMPILER_TARGET:STRING=i686-pc-windows-gnu",
            "-DUSE_ZLIB_NG_ON_X86=ON"
        )
    },
    @{
        Name = "legacy-x86"
        StagePackage = "$stagePackagePrefix-legacy-x86"
        BuildDir = Join-Path $clamavRoot "build-x86-mingw-winxp"
        UseMingw32 = $true
        ConfigureArgs = @(
            "-S", $clamavRoot,
            "-B", (Join-Path $clamavRoot "build-x86-mingw-winxp"),
            "-G", "MinGW Makefiles",
            "-DCMAKE_MAKE_PROGRAM=mingw32-make",
            "-DCMAKE_C_COMPILER=gcc",
            "-DCMAKE_CXX_COMPILER=g++",
            "-DCMAKE_RC_COMPILER=windres",
            "-DCMAKE_C_FLAGS=-std=gnu11 -Wno-error=implicit-function-declaration",
            "-DENABLE_LEGACY=ON",
            "-DCLAMWIN_SHELLEXT_UNICODE=ON",
            "-DRUST_COMPILER_TARGET:STRING=i686-pc-windows-gnu",
            "-DUSE_ZLIB_NG_ON_X86=ON"
        )
    },
    @{
        Name = "legacy-x64"
        StagePackage = "$stagePackagePrefix-legacy-x64"
        BuildDir = Join-Path $clamavRoot "build-x64"
        UseMingw32 = $false
        ConfigureArgs = @(
            "-S", $clamavRoot,
            "-B", (Join-Path $clamavRoot "build-x64"),
            "-G", "MinGW Makefiles",
            "-DCMAKE_MAKE_PROGRAM=mingw32-make",
            "-DCMAKE_C_COMPILER=gcc",
            "-DCMAKE_CXX_COMPILER=g++",
            "-DCMAKE_RC_COMPILER=windres",
            "-DCMAKE_C_FLAGS=-std=gnu11 -Wno-error=implicit-function-declaration",
            "-DENABLE_LEGACY=ON",
            "-DCLAMWIN_SHELLEXT_UNICODE=ON",
            "-DRUST_COMPILER_TARGET:STRING=x86_64-pc-windows-gnu"
        )
    },
    @{
        Name = "x64"
        StagePackage = "$stagePackagePrefix-x64"
        BuildDir = Join-Path $clamavRoot "build-gui"
        UseMingw32 = $false
        ConfigureArgs = @(
            "-S", $clamavRoot,
            "-B", (Join-Path $clamavRoot "build-gui"),
            "-G", "MinGW Makefiles",
            "-DCMAKE_MAKE_PROGRAM=mingw32-make",
            "-DCMAKE_C_COMPILER=gcc",
            "-DCMAKE_CXX_COMPILER=g++",
            "-DCMAKE_RC_COMPILER=windres",
            "-DCMAKE_C_FLAGS=-std=gnu11 -Wno-error=implicit-function-declaration",
            "-DENABLE_LEGACY=OFF",
            "-DCLAMWIN_SHELLEXT_UNICODE=ON",
            "-DRUST_COMPILER_TARGET:STRING=x86_64-pc-windows-gnu"
        )
    }
)

$clamavTargets = @("clambc", "clamscan", "freshclam", "sigtool", "clamd", "clamdscan", "clamdtop", "clamwin", "clamwin_shell_extension")
$copyNames = @(
    "clambc.exe",
    "clamscan.exe",
    "freshclam.exe",
    "sigtool.exe",
    "clamd.exe",
    "clamdscan.exe",
    "clamdtop.exe",
    "libclamav.dll",
    "libfreshclam.dll",
    "clamwin.exe",
    "libExpShell.dll"
)

if (-not $SkipBuild) {
    foreach ($p in $profiles) {
        if (!(Test-Path $p.BuildDir)) {
            throw "Build directory missing for profile $($p.Name): $($p.BuildDir)"
        }
        $expectedRustTarget = Get-ExpectedRustTarget -ConfigureArgs $p.ConfigureArgs
        Invoke-ConfigureProfile -Profile $p
        Invoke-BuildTarget -BuildDir $p.BuildDir -Targets $clamavTargets -UseMingw32 $p.UseMingw32 -RustTarget $expectedRustTarget
    }
}

$setupNodbExe = Invoke-BuildSetupNodb -ISToolExe $isToolExe -SetupDir $setupDir -SetupScript $setupNodbIss -SetupOutputDir $setupOutputDir

foreach ($p in $profiles) {
    $pkgPath = Join-Path $stageRoot $p.StagePackage
    if (!(Test-Path $pkgPath)) {
        throw "Stage package folder missing: $pkgPath"
    }

    Write-Host "[sync] profile=$($p.Name) stage=$pkgPath"

    foreach ($name in $copyNames) {
        $src = Join-Path $p.BuildDir $name
        $dst = Join-Path $pkgPath $name

        if (Test-Path $src) {
            Copy-Item $src $dst -Force
        }
        else {
            Write-Warning "Missing source binary for $($p.Name): $src"
        }
    }

    $pkgRegPath = Join-Path $pkgPath "cw_shell_extension.reg"
    Copy-Item $shellExtRegTemplate $pkgRegPath -Force
    Write-Host "[sync] copied shell extension reg to $pkgRegPath"

    if ($p.Name -eq "legacy-x86") {
        $xpDeployBat = Join-Path $pkgPath "xp-refresh-clamav-bin.bat"
        Copy-Item $xpRefreshBatTemplate $xpDeployBat -Force
        Write-Host "[sync] wrote XP refresh script to $xpDeployBat"
    }

    $pkgCertDir = Join-Path $pkgPath "certs"
    New-Item -ItemType Directory -Force $pkgCertDir | Out-Null
    Copy-Item $certSrc (Join-Path $pkgCertDir "clamav.crt") -Force

    $pkgDbDir = Join-Path $pkgPath "db"
    New-Item -ItemType Directory -Force $pkgDbDir | Out-Null
    foreach ($dbFile in $dbFileList) {
        Copy-Item $dbFile.FullName (Join-Path $pkgDbDir $dbFile.Name) -Force
    }
}

$topCertDir = Join-Path $stageRoot "certs"
New-Item -ItemType Directory -Force $topCertDir | Out-Null
Copy-Item $certSrc (Join-Path $topCertDir "clamav.crt") -Force

$topDbDir = Join-Path $stageRoot "db"
New-Item -ItemType Directory -Force $topDbDir | Out-Null
foreach ($dbFile in $dbFileList) {
    Copy-Item $dbFile.FullName (Join-Path $topDbDir $dbFile.Name) -Force
}

if (-not [string]::IsNullOrWhiteSpace($setupNodbExe) -and (Test-Path $setupNodbExe)) {
    $setupStagePath = Join-Path $stageRoot (Split-Path $setupNodbExe -Leaf)
    Copy-Item $setupNodbExe $setupStagePath -Force
    Write-Host "[sync] staged setup-nodb installer to $setupStagePath"
}

$isoBuilt = $false
if ($GenerateIso) {
    $stageMsys = Convert-ToMsysPath -WindowsPath $stageRoot
    $isoMsys = Convert-ToMsysPath -WindowsPath $isoPath

    Write-Host "[iso] building $isoPath"
    & $bashExe -lc "xorriso -as mkisofs -J -R -V CLAMWIN_ALL_OS -o '$isoMsys' '$stageMsys'"
    if ($LASTEXITCODE -ne 0) {
        throw "ISO build failed (exit $LASTEXITCODE)"
    }

    if ($MountVm.Count -gt 0) {
        $vbox = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
        if (!(Test-Path $vbox)) {
            throw "VBoxManage not found: $vbox"
        }

        foreach ($vm in $MountVm) {
            Write-Host "[mount] vm=$vm iso=$isoPath"
            & $vbox storageattach $vm --storagectl IDE --port 1 --device 0 --type dvddrive --medium $isoPath
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to mount ISO on VM '$vm'"
            }
        }
    }

    $isoBuilt = $true
}
elseif ($MountVm.Count -gt 0) {
    throw "MountVm requires -GenerateIso"
}

if ($isoBuilt) {
    $isoInfo = Get-Item $isoPath
    Write-Host "[done] ISO ready: $($isoInfo.FullName) ($([Math]::Round($isoInfo.Length / 1MB, 2)) MB)"
}
else {
    Write-Host "[done] setup/staging complete (ISO not generated; use -GenerateIso)"
}
