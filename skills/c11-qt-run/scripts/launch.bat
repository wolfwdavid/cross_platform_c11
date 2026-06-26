@echo off
REM Launch the Windows c11-qt app with a PATH that lets the panes' spawned cmd.exe
REM shells work. Launching with a Unix-style PATH (e.g. from Git Bash) breaks every
REM pane shell with "'cmd.exe' is not recognized".
REM
REM Qt runtime: override by setting C11_QT_BIN; defaults to the pinned CI Qt.
REM Resolves the repo from this script's location (skills\c11-qt-run\scripts\).

if "%C11_QT_BIN%"=="" set "C11_QT_BIN=C:\Qt\6.11.1\msvc2022_64\bin"
set "PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0;%C11_QT_BIN%"

set "C11_BIN=%~dp0..\..\..\c11-qt\build\bin"
if not exist "%C11_BIN%\c11.exe" (
    echo ERROR: %C11_BIN%\c11.exe not found. Build first: c11-qt\build_msvc.bat
    exit /b 1
)

cd /d "%C11_BIN%"
echo Launching c11-qt app from "%C11_BIN%"
start "" "%C11_BIN%\c11.exe"
