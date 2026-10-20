; Windows installer of Drum Programmer (AP11), built with Inno Setup 6:
;   iscc /DAppVersion=1.0.0 /DBuildDir=<release build dir> /DOutputDir=<dir> packaging\windows\DrumProgrammer.iss
; The program is one exe with the static MSVC runtime; samples, icons and texts are built into it.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build"
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
AppId={{6F0D2C1E-8E43-4C1B-9B57-3D1A2A9F5B21}
AppName=Drum Programmer
AppVersion={#AppVersion}
AppPublisher=ladych
AppPublisherURL=https://github.com/ladych/DrumProgrammer
AppSupportURL=https://github.com/ladych/DrumProgrammer/issues
DefaultDirName={autopf}\Drum Programmer
DefaultGroupName=Drum Programmer
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=DrumProgrammer-{#AppVersion}-Setup
SetupIconFile=..\..\resources\AppIcon\DrumProgrammer.ico
UninstallDisplayIcon={app}\Drum Programmer.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequiredOverridesAllowed=dialog
ChangesAssociations=yes

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "association"; Description: "{cm:AssocFileExtension,Drum Programmer,.dpp}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#BuildDir}\src\DrumProgrammer_artefacts\Release\Drum Programmer.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Drum Programmer"; Filename: "{app}\Drum Programmer.exe"
Name: "{group}\{cm:UninstallProgram,Drum Programmer}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Drum Programmer"; Filename: "{app}\Drum Programmer.exe"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.dpp\OpenWithProgids"; ValueType: string; ValueName: "DrumProgrammer.Project"; ValueData: ""; Flags: uninsdeletevalue; Tasks: association
Root: HKA; Subkey: "Software\Classes\DrumProgrammer.Project"; ValueType: string; ValueName: ""; ValueData: "Drum Programmer Project"; Flags: uninsdeletekey; Tasks: association
Root: HKA; Subkey: "Software\Classes\DrumProgrammer.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\Drum Programmer.exe,0"; Tasks: association
Root: HKA; Subkey: "Software\Classes\DrumProgrammer.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Drum Programmer.exe"" ""%1"""; Tasks: association

[Run]
Filename: "{app}\Drum Programmer.exe"; Description: "{cm:LaunchProgram,Drum Programmer}"; Flags: nowait postinstall skipifsilent
