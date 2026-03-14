param(
    [switch]$RealTools,
    [switch]$FreshclamUpdate,
    [switch]$FreshclamNegative,
    [switch]$BuildOnly,
    [switch]$NoStopClamwin,
    [string]$BuildDir = "build-gui"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildPath = Join-Path $repoRoot $BuildDir

if (-not (Test-Path $buildPath))
{
    throw ('Build directory ''{0}'' does not exist. Configure the GUI build first with: cmake -S . -B {1} -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=mingw32-make' -f $buildPath, $BuildDir)
}

$pathPrefixes = @(
    "C:\Users\alexc\.cargo\bin",
    "C:\msys64\mingw64\bin",
    "C:\Program Files\CMake\bin"
)

$existingPath = @()
foreach ($pathPrefix in $pathPrefixes)
{
    if (Test-Path $pathPrefix)
    {
        $existingPath += $pathPrefix
    }
}

if ($existingPath.Count -gt 0)
{
    $env:PATH = (($existingPath -join ";") + ";" + $env:PATH)
}

if ($FreshclamUpdate -or $FreshclamNegative)
{
    $RealTools = $true
}

if ($RealTools)
{
    $env:CLAMWIN_REAL_TOOLS = "1"
}
else
{
    Remove-Item Env:CLAMWIN_REAL_TOOLS -ErrorAction SilentlyContinue
}

if ($FreshclamUpdate)
{
    $env:CLAMWIN_REAL_FRESHCLAM_UPDATE = "1"
}
else
{
    Remove-Item Env:CLAMWIN_REAL_FRESHCLAM_UPDATE -ErrorAction SilentlyContinue
}

if ($FreshclamNegative)
{
    $env:CLAMWIN_REAL_FRESHCLAM_NEGATIVE = "1"
}
else
{
    Remove-Item Env:CLAMWIN_REAL_FRESHCLAM_NEGATIVE -ErrorAction SilentlyContinue
}

if (-not $NoStopClamwin)
{
    Stop-Process -Name "clamwin" -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 400
}

$target = if ($BuildOnly)
{
    "clamwin_gui_test"
}
elseif ($RealTools)
{
    "clamwin_gui_check_real_tools"
}
else
{
    "clamwin_gui_check"
}

Write-Host "Repository:" $repoRoot
Write-Host "Build directory:" $buildPath
Write-Host "Target:" $target
if ($RealTools)
{
    Write-Host "Real tool smoke tests enabled"
}
if ($FreshclamUpdate)
{
    Write-Host "FreshClam update probe enabled"
}
if ($FreshclamNegative)
{
    Write-Host "FreshClam negative probe enabled"
}

cmake --build $buildPath --target $target