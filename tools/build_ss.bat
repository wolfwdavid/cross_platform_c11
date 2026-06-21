@echo off
REM Build the Windows screenshot validation helper (tools/screenshot.cpp).
REM Output: tools/screenshot.exe. Run from anywhere; cd's to its own dir.
set "PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0;C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" || exit /b 1
cd /d "%~dp0"
cl /nologo /EHsc /O2 screenshot.cpp /Fe:screenshot.exe
exit /b %ERRORLEVEL%
