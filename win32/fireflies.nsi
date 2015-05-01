; fireflies.nsi - 

!define VERSION 2.05
Name "Fireflies ${VERSION}"
OutFile "fireflies-scr-${VERSION}.exe"

InstallDir $WINDIR

; The stuff to install
Section "Example2 (required)"
    SectionIn RO

    MessageBox MB_YESNO|MB_ICONINFORMATION "This will install the fireflies screensaver.  Click Yes to proceed." IDYES yesinstall
	Quit
    yesinstall:
    SetOutPath $INSTDIR
    File "..\src\fireflies.scr"

    ; Write the installation path into the registry
    WriteRegStr HKLM SOFTWARE\NSIS_Example2 "Install_Dir" "$INSTDIR"

    ; Write the uninstall keys for Windows
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fireflies" "DisplayName" "Fireflies Screensaver (remove only)"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fireflies" "UninstallString" '"$INSTDIR\fire-un.exe"'
    WriteUninstaller "fire-un.exe"
SectionEnd

;--------------------------------
; Uninstaller

UninstallText "This will uninstall Fireflies. Hit next to continue."

Section "Uninstall"
    ; remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fireflies"
    DeleteRegKey HKCU "Software\Fireflies"

    ; remove files and uninstaller
    Delete $INSTDIR\fireflies.scr
    Delete $INSTDIR\fire-un.exe
SectionEnd
