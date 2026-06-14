; c11 NSIS Installer Script
; Requires NSIS 3.0+

!include "MUI2.nsh"
!include "FileFunc.nsh"

; General
Name "c11"
OutFile "c11-setup.exe"
InstallDir "$PROGRAMFILES64\c11"
InstallDirRegKey HKLM "Software\Stage11\c11" "InstallDir"
RequestExecutionLevel admin

; Version info
!define VERSION "0.1.0"
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "c11"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "c11 terminal multiplexer"
VIAddVersionKey "LegalCopyright" "Stage 11"

; UI
!define MUI_ICON "c11.ico"
!define MUI_ABORTWARNING

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; Installation
Section "c11 (required)" SecMain
    SectionIn RO

    SetOutPath "$INSTDIR"

    ; Main application
    File "c11.exe"

    ; CLI tool
    File "c11-cli.exe"

    ; Qt runtime DLLs (deployed by windeployqt)
    File /r "platforms\*.*"
    File /r "styles\*.*"
    File /r "imageformats\*.*"
    File /r "tls\*.*"
    File "*.dll"

    ; Qt WebEngine
    File /r "QtWebEngineProcess.exe"
    File /r "resources\*.*"
    File /r "translations\*.*"

    ; Write registry keys
    WriteRegStr HKLM "Software\Stage11\c11" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Stage11\c11" "Version" "${VERSION}"

    ; Uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Add/Remove Programs entry
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "DisplayName" "c11 terminal multiplexer"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "Publisher" "Stage 11"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "NoRepair" 1

    ; Compute installed size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11" \
        "EstimatedSize" "$0"
SectionEnd

; Start Menu shortcuts
Section "Start Menu Shortcut" SecStartMenu
    CreateDirectory "$SMPROGRAMS\c11"
    CreateShortCut "$SMPROGRAMS\c11\c11.lnk" "$INSTDIR\c11.exe"
    CreateShortCut "$SMPROGRAMS\c11\Uninstall.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

; Add to PATH
Section "Add to PATH" SecPath
    ; Add install dir to system PATH for the CLI
    EnVar::SetHKLM
    EnVar::AddValue "PATH" "$INSTDIR"
SectionEnd

; Uninstaller
Section "Uninstall"
    ; Remove files
    RMDir /r "$INSTDIR"

    ; Remove Start Menu shortcuts
    RMDir /r "$SMPROGRAMS\c11"

    ; Remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\c11"
    DeleteRegKey HKLM "Software\Stage11\c11"

    ; Remove from PATH
    EnVar::SetHKLM
    EnVar::DeleteValue "PATH" "$INSTDIR"
SectionEnd
