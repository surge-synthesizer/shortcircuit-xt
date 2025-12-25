#define MyAppPublisher "Surge Synth Team"
#define MyAppURL "https://www.surge-synth-team.org/"
#define MyAppName "Shortcircuit XT"
#define MyAppNameCondensed "ShortcircuitXT"
#define MyID "8BDBC849-F102-44A0-9BFA-B28556BDE40B"

#ifndef MyAppVersion
#define MyAppVersion "0.0.0"
#endif

;uncomment these two lines if building the installer locally!
;#define SCXT_SRC "..\..\"
;#define SCXT_BIN "..\..\build\"

[Setup]
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
AppId={#MyID}
AppName={#MyAppName}
AppVerName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}\
DefaultGroupName={#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
AlwaysShowDirOnReadyPage=yes
LicenseFile={#SCXT_SRC}\LICENSE
OutputBaseFilename={#MyAppNameCondensed}-{#MyAppVersion}-Windows-64bit-setup
SetupIconFile={#SCXT_SRC}\resources\installer_win\scxt.ico
UninstallDisplayIcon={uninstallexe}
UsePreviousAppDir=yes
Compression=lzma2/ultra64
LZMAUseSeparateProcess=yes
LZMANumFastBytes=273
LZMADictionarySize=1048576
LZMANumBlockThreads=6
SolidCompression=yes
UninstallFilesDir={autoappdata}\{#MyAppName}\uninstall
CloseApplicationsFilter=*.exe,*.vst3
WizardStyle=modern dynamic
WizardSizePercent=100
WizardImageAlphaFormat=defined
WizardSmallImageFile={#SCXT_SRC}\resources\installer_win\blank.png
WizardSmallImageFileDynamicDark={#SCXT_SRC}\resources\installer_win\blank.png
WizardImageFile={#SCXT_SRC}\resources\installer_win\blank.png
WizardImageFileDynamicDark={#SCXT_SRC}\resources\installer_win\blank.png

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation"
Name: "clap"; Description: "CLAP installation"
Name: "vst3"; Description: "VST3 installation"
Name: "standalone"; Description: "Standalone installation"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "CLAP"; Description: "{#MyAppName} CLAP (64-bit)"; Types: full clap custom
Name: "VST3"; Description: "{#MyAppName} VST3 (64-bit)"; Types: full vst3 custom
Name: "SA"; Description: "{#MyAppName} Standalone (64-bit)"; Types: full standalone custom

[Files]
Source: "{#SCXT_BIN}\shortcircuit-products\{#MyAppName}.clap"; DestDir: "{autocf}\CLAP\{#MyAppPublisher}\"; Components: CLAP; Flags: ignoreversion
Source: "{#SCXT_BIN}\shortcircuit-products\{#MyAppName}.vst3\*"; DestDir: "{autocf}\VST3\{#MyAppPublisher}\{#MyAppName}.vst3\"; Components: VST3; Flags: ignoreversion recursesubdirs
Source: "{#SCXT_BIN}\shortcircuit-products\{#MyAppName}.exe"; DestDir: "{app}"; Components: SA; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"; Flags: createonlyiffileexists
