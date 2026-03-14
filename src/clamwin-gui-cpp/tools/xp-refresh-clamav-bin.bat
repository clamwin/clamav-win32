@echo off
setlocal

set "SRC=%~dp0"
set "DST=C:\clamav\bin"

echo [1/3] Stopping ClamWin and Explorer...
where taskkill >nul 2>&1
if %ERRORLEVEL%==0 (
    taskkill /F /IM clamwin.exe >nul 2>&1
    taskkill /F /IM explorer.exe >nul 2>&1
) else (
    tskill clamwin >nul 2>&1
    tskill explorer >nul 2>&1
)
ping 127.0.0.1 -n 2 >nul

echo [2/3] Copying package contents to %DST%...
if not exist "%DST%" mkdir "%DST%"
xcopy "%SRC%*" "%DST%\" /E /I /Y /H /R /K >nul
reg add "HKCU\Software\ClamWin" /v Path /t REG_SZ /d "%DST%" /f >nul 2>&1

echo [3/3] Restarting Explorer and ClamWin...
start "" explorer.exe
if exist "%DST%\clamwin.exe" start "" "%DST%\clamwin.exe"

echo Done.
endlocal
