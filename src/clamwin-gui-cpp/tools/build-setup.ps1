param(
    [Alias('SkipClamBuild','SkipClamAvBuild')]
    [switch]$SkipBuild,
    [switch]$GenerateIso,
    [switch]$IncludeFulldbInstaller,
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

    $candidates = @(Get-ChildItem -Path $StageRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like "clamav-*-$Suffix" })

    if ($candidates.Count -eq 0) {
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

function Get-LatestSetupExe {
    param(
        [Parameter(Mandatory = $true)][string]$SetupOutputDir,
        [Parameter(Mandatory = $true)][string]$SetupScript
    )

    if (-not (Test-Path $SetupOutputDir)) {
        return ""
    }

    $scriptLeaf = (Split-Path $SetupScript -Leaf).ToLowerInvariant()
    if ($scriptLeaf -eq "setup-nodb.iss") {
        $pattern = '(^clamwin-.*-setup-nodb\.exe$)|(^Setup-nodb\.exe$)'
    }
    else {
        $pattern = '(^clamwin-.*-setup\.exe$)|(^Setup\.exe$)'
    }

    $latest = Get-ChildItem -Path $SetupOutputDir -File |
        Where-Object { $_.Name -match $pattern } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $latest) {
        return ""
    }

    return $latest.FullName
}

function Invoke-BuildSetup {
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

    $setupExe = Get-LatestSetupExe -SetupOutputDir $SetupOutputDir -SetupScript $SetupScript
    if ([string]::IsNullOrWhiteSpace($setupExe) -or -not (Test-Path $setupExe)) {
        throw "Setup output not found in: $SetupOutputDir"
    }

    Write-Host "[setup] built $setupExe"
    return $setupExe
}

function Invoke-PrepareBundledDatabases {
    param(
        [Parameter(Mandatory = $true)][string]$FreshclamExe,
        [Parameter(Mandatory = $true)][string]$CvdDir,
        [Parameter(Mandatory = $true)][string]$CertSource
    )

    if (-not (Test-Path $FreshclamExe)) {
        throw "freshclam.exe not found: $FreshclamExe"
    }
    if (-not (Test-Path $CertSource)) {
        throw "clamav.crt not found: $CertSource"
    }

    $freshclamDir = Split-Path $FreshclamExe -Parent
    $freshclamCertDir = Join-Path $freshclamDir "certs"
    New-Item -ItemType Directory -Force $freshclamCertDir | Out-Null
    Copy-Item $CertSource (Join-Path $freshclamCertDir "clamav.crt") -Force

    if (Test-Path $CvdDir) {
        Get-ChildItem -Path $CvdDir -File -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
    }
    else {
        New-Item -ItemType Directory -Force $CvdDir | Out-Null
    }

    $freshclamConf = Join-Path $CvdDir "freshclam-build-setup.conf"
    $freshclamLog = Join-Path $CvdDir "freshclam-build-setup.log"
    $conf = @(
        "DatabaseDirectory $CvdDir",
        "UpdateLogFile $freshclamLog",
        "DatabaseMirror database.clamav.net"
    )
    Set-Content -Path $freshclamConf -Value $conf -Encoding ascii

    Write-Host "[db] downloading bundled CVD files via $FreshclamExe"
    & $FreshclamExe --config-file "$freshclamConf"
    if ($LASTEXITCODE -ne 0) {
        throw "freshclam failed while preparing setup CVD files (exit $LASTEXITCODE)"
    }

    $required = @("main.cvd", "daily.cvd", "bytecode.cvd")
    foreach ($name in $required) {
        $path = Join-Path $CvdDir $name
        if (-not (Test-Path $path)) {
            throw "Missing required CVD file for Setup.iss: $path"
        }
    }

    Write-Host "[db] bundled CVD files prepared in $CvdDir"
}

$clamavRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\..\.."))
$repoRoot = Split-Path $clamavRoot -Parent
$isoPayloadRoot = Join-Path $repoRoot "binaries\iso-payload"
$isoPath = Join-Path $repoRoot "binaries\clamwin-all-os.iso"
$projectVersion = Get-ProjectVersion -CMakeListsPath (Join-Path $clamavRoot "CMakeLists.txt")
$isToolExe = "C:\Program Files (x86)\ISTool\ISTool.exe"
$setupDir = Join-Path $clamavRoot "src\clamwin-gui-cpp\setup"
$setupIss = Join-Path $setupDir "Setup.iss"
$setupNodbIss = Join-Path $setupDir "Setup-nodb.iss"
$setupOutputDir = Join-Path $setupDir "Output"
$setupCvdDir = Join-Path $setupDir "cvd"
$bashExe = "C:\msys64\usr\bin\bash.exe"
$clamavCert = Join-Path $clamavRoot "clamav\certs\clamav.crt"

if (!(Test-Path $bashExe)) {
    throw "MSYS bash not found: $bashExe"
}

if ($SkipBuild) {
    Write-Host "[build] SkipBuild enabled: skipping configure/compile; installer will still be rebuilt"
}
Write-Host ("[version] detected clamav-win32 version: {0}" -f $projectVersion)

$profiles = @(
    @{
        Name = "win9x"
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

$setupNodbExe = Invoke-BuildSetup -ISToolExe $isToolExe -SetupDir $setupDir -SetupScript $setupNodbIss -SetupOutputDir $setupOutputDir
$setupExe = ""
if ($IncludeFulldbInstaller) {
    $freshclamForSetup = Join-Path (Join-Path $clamavRoot "build-gui") "freshclam.exe"
    Invoke-PrepareBundledDatabases -FreshclamExe $freshclamForSetup -CvdDir $setupCvdDir -CertSource $clamavCert
    $setupExe = Invoke-BuildSetup -ISToolExe $isToolExe -SetupDir $setupDir -SetupScript $setupIss -SetupOutputDir $setupOutputDir
}

if (Test-Path $isoPayloadRoot) {
    Remove-Item $isoPayloadRoot -Recurse -Force
}
New-Item -ItemType Directory -Force $isoPayloadRoot | Out-Null

$setupNodbPayloadPath = Join-Path $isoPayloadRoot (Split-Path $setupNodbExe -Leaf)
Copy-Item $setupNodbExe $setupNodbPayloadPath -Force
Write-Host "[iso] payload prepared: $setupNodbPayloadPath"
if ($IncludeFulldbInstaller -and -not [string]::IsNullOrWhiteSpace($setupExe) -and (Test-Path $setupExe)) {
    $setupPayloadPath = Join-Path $isoPayloadRoot (Split-Path $setupExe -Leaf)
    Copy-Item $setupExe $setupPayloadPath -Force
    Write-Host "[iso] payload prepared: $setupPayloadPath"
}

$isoBuilt = $false
if ($GenerateIso) {
    $payloadMsys = Convert-ToMsysPath -WindowsPath $isoPayloadRoot
    $isoMsys = Convert-ToMsysPath -WindowsPath $isoPath

    Write-Host "[iso] building $isoPath"
    & $bashExe -lc "xorriso -as mkisofs -J -R -V CLAMWIN_ALL_OS -o '$isoMsys' '$payloadMsys'"
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
    if ($IncludeFulldbInstaller -and -not [string]::IsNullOrWhiteSpace($setupExe)) {
        Write-Host "[done] nodb setup ready: $setupNodbExe ; fulldb setup ready: $setupExe (ISO not generated; use -GenerateIso)"
    }
    else {
        Write-Host "[done] nodb setup ready: $setupNodbExe (ISO not generated; use -GenerateIso)"
    }
}
