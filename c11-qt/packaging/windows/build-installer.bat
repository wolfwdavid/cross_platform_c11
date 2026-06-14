@echo off
REM Build c11 Windows installer
REM Prerequisites: CMake, Qt 6.7+, NSIS 3.0+
REM Usage: build-installer.bat [build-dir]

setlocal

set SCRIPT_DIR=%~dp0
set REPO_ROOT=%SCRIPT_DIR%..\..
set BUILD_DIR=%1
if "%BUILD_DIR%"=="" set BUILD_DIR=%REPO_ROOT%\build-win

echo === Building c11 for Windows ===

REM Configure
cmake -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release "%REPO_ROOT%"
if errorlevel 1 goto :error

REM Build
cmake --build "%BUILD_DIR%" --config Release -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 goto :error

REM Deploy Qt DLLs
set DEPLOY_DIR=%BUILD_DIR%\deploy
mkdir "%DEPLOY_DIR%" 2>nul
copy "%BUILD_DIR%\bin\Release\c11.exe" "%DEPLOY_DIR%\" >nul
copy "%BUILD_DIR%\cli\Release\c11.exe" "%DEPLOY_DIR%\c11-cli.exe" >nul

REM Run windeployqt
windeployqt --release --no-translations "%DEPLOY_DIR%\c11.exe"
if errorlevel 1 (
    echo WARNING: windeployqt failed. Installer may be incomplete.
)

REM Copy NSIS script
copy "%SCRIPT_DIR%\installer.nsi" "%DEPLOY_DIR%\" >nul

REM Build NSIS installer
if exist "%PROGRAMFILES(X86)%\NSIS\makensis.exe" (
    "%PROGRAMFILES(X86)%\NSIS\makensis.exe" "%DEPLOY_DIR%\installer.nsi"
    echo === Installer created: %DEPLOY_DIR%\c11-setup.exe ===
) else (
    echo WARNING: NSIS not found. Deploy directory ready at %DEPLOY_DIR%
)

goto :done

:error
echo === Build failed ===
exit /b 1

:done
echo === Done ===
