; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!
; $Id: cuemol2.iss,v 1.15 2011/04/03 12:50:02 rishitani Exp $

; #define CueMolReleaseID "@VERSION_RELEASE_ID@"
; #define CueMolBuildID "@VERSION_BUILD_ID@"

#define WinBuildDir "..\xul_Release"
; #define WinBuildDir "..\xul_Debug"
#define XulGUIDir "..\..\src\xul_gui"

#define SysDLLDir PROJ_DIR+"\bin"
#define XulRTDir PROJ_DIR+"\xulrunner\xulrunner2-sdk\bin"

#define BoostVer "vc90-mt-1_44"

#define CueMolVer CueMolReleaseID+"."+CueMolBuildID

[Setup]
AppName=CueMol2
AppVerName=CueMol2 {#CueMolReleaseID} build {#CueMolBuildID}
AppPublisher=BKR Laboratory
AppPublisherURL=http://www.cuemol.org/
AppSupportURL=http://www.cuemol.org/
AppUpdatesURL=http://www.cuemol.org/
DefaultDirName={pf}\CueMol 2.0
DefaultGroupName=CueMol 2.0

AllowNoIcons=yes
OutputDir=..\build
#ifdef PovBundleDir
OutputBaseFilename=cuemol2-{#CueMolVer}-win32-povray-setup
#else
OutputBaseFilename=cuemol2-{#CueMolVer}-win32-setup
#endif
; SetupIconFile=
; Compression=lzma
SolidCompression=yes
PrivilegesRequired=none
ChangesAssociations=yes

WizardImageFile=compiler:WIZMODERNIMAGE-IS.BMP
WizardSmallImageFile=compiler:WIZMODERNSMALLIMAGE-IS.BMP

[Tasks]
Name: desktopicon; Description: Create a &desktop icon; GroupDescription: Additional icons:;
Name: desktopicon\common; Description: For all users; GroupDescription: Additional icons:; Flags: exclusive
Name: desktopicon\user; Description: "For the current user only"; GroupDescription: "Additional icons:"; Flags: exclusive unchecked
Name: quicklaunchicon; Description: "Create a &Quick Launch icon"; GroupDescription: "Additional icons:"; Flags: unchecked
Name: fileassoc_qsc; Description: "{cm:AssocFileExtension,CueMol2 scene file,.qsc}"
Name: activexctrl; Description: "Register the CueMol2 &ActiveX control"; GroupDescription: "ActiveX Control:";  Flags: unchecked

[Files]
; Main executables
Source: {#WinBuildDir}\cuemol2.exe; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\jsbr.dll; DestDir: {app}; Flags: ignoreversion
; Source: {#WinBuildDir}\js.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\qlib.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\qsys.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\molanl.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\surface.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\symm.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\xtal.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\lwview.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\mdtools.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\render.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\anim.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\blendpng.exe; DestDir: {app}; Flags: ignoreversion

Source: {#WinBuildDir}\components\xpcqm2.dll; DestDir: {app}\components; Flags: ignoreversion
Source: "{#WinBuildDir}\components\*.xpt"; DestDir: {app}\components; Flags: ignoreversion

Source: {#WinBuildDir}\qmlibpng.dll; DestDir: {app}; Flags: ignoreversion
Source: {#WinBuildDir}\qmzlib.dll; DestDir: {app}; Flags: ignoreversion

; Runtime (C/C++), BOOST, and other DLLs
Source: {#SysDLLDir}\msvcp90.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\msvcr90.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\msvcm90.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\Microsoft.VC90.CRT.manifest; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\boost_thread-{#BoostVer}.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\boost_date_time-{#BoostVer}.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\boost_system-{#BoostVer}.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\boost_filesystem-{#BoostVer}.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\libfftw3f-3.dll; DestDir: {app}; Flags: ignoreversion
Source: {#SysDLLDir}\glew32.dll; DestDir: {app}; Flags: ignoreversion

; Source: {#SysDLLDir}\libpng15.dll; DestDir: {app}; Flags: ignoreversion
; Source: {#SysDLLDir}\zlib1.dll; DestDir: {app}; Flags: ignoreversion

; XUL files
Source: {#XulGUIDir}\application.ini; DestDir: {app}; Flags: ignoreversion
Source: {#XulGUIDir}\chrome.manifest; DestDir: {app}; Flags: ignoreversion

Source: {#WinBuildDir}\chrome\cuemol2.manifest; DestDir: "{app}\chrome"; Flags: ignoreversion
Source: {#WinBuildDir}\chrome\cuemol2.jar; DestDir: "{app}\chrome"; Flags: ignoreversion

Source: "{#XulGUIDir}\chrome\icons\*"; Excludes: "*~,CVS,.svn"; DestDir: "{app}\chrome\icons"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#XulGUIDir}\defaults\*"; Excludes: "*~,CVS,.svn,debug-prefs.js"; DestDir: "{app}\defaults"; Flags: ignoreversion recursesubdirs createallsubdirs
; Source: "{#XulGUIDir}\extensions\*"; Excludes: "*~,CVS,.svn"; DestDir: "{app}\chrome"; Flags: ignoreversion recursesubdirs createallsubdirs

; XUL files (modules and jetpack)
Source: {#XulGUIDir}\harness-options.json; DestDir: {app}; Flags: ignoreversion
Source: "{#XulGUIDir}\components\*.js"; DestDir: {app}\components; Flags: ignoreversion
Source: "{#XulGUIDir}\resources\addon-kit-lib\*"; Excludes: "*~,CVS,.svn"; DestDir: "{app}\resources\addon-kit-lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#XulGUIDir}\resources\api-utils-lib\*"; Excludes: "*~,CVS,.svn"; DestDir: "{app}\resources\api-utils-lib"; Flags: ignoreversion recursesubdirs createallsubdirs

Source: "{#XulGUIDir}\resources\cuemol-wrappers\*"; Excludes: "*~,CVS,.svn"; DestDir: "{app}\resources\cuemol-wrappers"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#XulGUIDir}\resources\cuemol2ui-lib\*"; Excludes: "*~,CVS,.svn"; DestDir: "{app}\resources\cuemol2ui-lib"; Flags: ignoreversion recursesubdirs createallsubdirs

; XULRunner runtime
Source: "{#XulRTDir}\*"; DestDir: "{app}\xulrunner23"; Flags: ignoreversion recursesubdirs createallsubdirs

; data files
Source: {#XulGUIDir}\sysconfig.xml; DestDir: {app}; Flags: ignoreversion
Source: {#XulGUIDir}\data\queptl.lin; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\queptl.prm; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\queptl.top; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\default_style.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\symop.dat; DestDir: {app}\data; Flags: ignoreversion

Source: {#XulGUIDir}\data\prot_top.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\prot_props.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\nucl_top.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\nucl_props.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\sugar_top.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\mono_top.xml; DestDir: {app}\data; Flags: ignoreversion
Source: {#XulGUIDir}\data\default_params.xml; DestDir: {app}\data; Flags: ignoreversion

Source: {#WinBuildDir}\data\shaders\*; Excludes: "*~,CVS,.svn"; DestDir: {app}\data\shaders; Flags: ignoreversion

; Source: {#XulGUIDir}\data\amb_all03chg_prot.xml; DestDir: {app}\data; Flags: ignoreversion

; POV-Ray bundle option
#ifdef PovBundleDir
Source: "{#PovBundleDir}\povray.exe"; DestDir: "{app}\povray\bin"; Flags: ignoreversion
Source: "{#PovBundleDir}\include\*"; DestDir: "{app}\povray\include"; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

; FFmpeg bundle option
#ifdef FFmpegBundleDir
Source: "{#FFmpegBundleDir}\bin\ffmpeg.exe"; DestDir: "{app}\ffmpeg\bin"; Flags: ignoreversion
#endif

; APBS/pdb2pqr bundle option
#ifdef APBSBundleDir
Source: "{#APBSBundleDir}\*"; DestDir: "{app}\apbs"; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

; ActiveX control
Source: {#WinBuildDir}\CueMol2Ctl.ocx;   DestDir: {app};   Flags: ignoreversion regserver;   Tasks: activexctrl
; Source: {#WinBuildDir}\CueMol2Ctl.ocx;   DestDir: {app};   Flags: ignoreversion regserver;   AfterInstall: RegisterServerXX('{app}\CueMol2Ctl.ocx');   Tasks: activexctrl

[Icons]
Name: {group}\CueMol; Filename: {app}\cuemol2.exe
Name: {userdesktop}\CueMol 2.0; Filename: {app}\cuemol2.exe; Tasks: desktopicon\user
Name: {commondesktop}\CueMol 2.0; Filename: {app}\cuemol2.exe; Tasks: desktopicon\common
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\CueMol; Filename: {app}\cuemol2.exe; Tasks: quicklaunchicon

[Registry]
; QSC document file
Root: HKCR; Subkey: ".qsc"; ValueType: string; ValueName: ""; ValueData: "CueMol2SceneFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "CueMol2SceneFile"; ValueType: string; ValueName: ""; ValueData: "CueMol2 Scene File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "CueMol2SceneFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cuemol2.exe,1"
Root: HKCR; Subkey: "CueMol2SceneFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\cuemol2.exe"" ""%1""" 
; QSL document file
Root: HKCR; Subkey: ".qsl"; ValueType: string; ValueName: ""; ValueData: "CueMol2LWSceneFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "CueMol2LWSceneFile"; ValueType: string; ValueName: ""; ValueData: "CueMol2 Light-weight Scene"; Flags: uninsdeletekey
Root: HKCR; Subkey: "CueMol2LWSceneFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\cuemol2.exe,2"
Root: HKCR; Subkey: "CueMol2LWSceneFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\cuemol2.exe"" ""%1""" 


