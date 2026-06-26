@echo off
REM Build the c11 Windows installer.
REM Prerequisites:
REM   - Visual Studio 2022 (MSVC) + CMake
REM   - Qt 6.7+ MSVC kit WITH the QtWebEngine module; its bin\ on PATH (windeployqt)
REM   - NSIS 3.0+ (optional, for the installer itself)
REM Usage: build-installer.bat [build-dir]

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set REPO_ROOT=%SCRIPT_DIR%..\..
set BUILD_DIR=%1
if "%BUILD_DIR%"=="" set BUILD_DIR=%REPO_ROOT%\build-win

echo === Building c11 for Windows ===

REM Configure + build (Release)
cmake -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release "%REPO_ROOT%"
if errorlevel 1 goto :error
cmake --build "%BUILD_DIR%" --config Release -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 goto :error

REM Locate the built GUI binary (multi-config VS puts it under bin\Release\;
REM single-config generators such as Ninja put it under bin\).
set APP_EXE=
if exist "%BUILD_DIR%\bin\Release\c11.exe" set APP_EXE=%BUILD_DIR%\bin\Release\c11.exe
if not defined APP_EXE if exist "%BUILD_DIR%\bin\c11.exe" set APP_EXE=%BUILD_DIR%\bin\c11.exe
if not defined APP_EXE (
    echo ERROR: could not find built c11.exe under %BUILD_DIR%\bin
    goto :error
)

REM Stage a clean deploy dir.
set DEPLOY_DIR=%BUILD_DIR%\deploy
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"
copy "%APP_EXE%" "%DEPLOY_DIR%\c11.exe" >nul

REM CLI binary (optional, path varies by generator).
if exist "%BUILD_DIR%\cli\Release\c11.exe" copy "%BUILD_DIR%\cli\Release\c11.exe" "%DEPLOY_DIR%\c11-cli.exe" >nul
if exist "%BUILD_DIR%\cli\c11.exe" copy "%BUILD_DIR%\cli\c11.exe" "%DEPLOY_DIR%\c11-cli.exe" >nul

REM Deploy the Qt runtime. windeployqt auto-detects Qt WebEngine and copies
REM QtWebEngineProcess.exe + the WebEngine resources/. We must NOT pass
REM --no-translations: WebEngine requires its locale .pak files
REM (translations\qtwebengine_locales\) or browser panels fail to load.
where windeployqt >nul 2>nul
if errorlevel 1 (
    echo ERROR: windeployqt not on PATH. Add your Qt kit's bin\ directory to PATH.
    goto :error
)
windeployqt --release "%DEPLOY_DIR%\c11.exe"
if errorlevel 1 (
    echo WARNING: windeployqt reported an error. Installer may be incomplete.
)

REM Software OpenGL fallback, used when no usable desktop GL driver is present.
for %%D in (windeployqt.exe) do set QT_BIN=%%~dp$PATH:D
if defined QT_BIN if exist "%QT_BIN%opengl32sw.dll" copy "%QT_BIN%opengl32sw.dll" "%DEPLOY_DIR%\" >nul

REM libghostty runtime (only present when built with -DC11_BUILD_GHOSTTY=ON as a DLL).
if exist "%BUILD_DIR%\ghostty.dll" copy "%BUILD_DIR%\ghostty.dll" "%DEPLOY_DIR%\" >nul

REM Build the NSIS installer.
copy "%SCRIPT_DIR%\installer.nsi" "%DEPLOY_DIR%\" >nul
if exist "%PROGRAMFILES(X86)%\NSIS\makensis.exe" (
    "%PROGRAMFILES(X86)%\NSIS\makensis.exe" "%DEPLOY_DIR%\installer.nsi"
    echo === Installer created in %DEPLOY_DIR% ===
) else (
    echo NSIS not found. Deploy directory ready at %DEPLOY_DIR%
)

goto :done

:error
echo === Build failed ===
exit /b 1

:done
echo === Done ===
endlocal
