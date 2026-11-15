@echo off

net session >nul 2>&1
if not %errorLevel% == 0 (
	echo Administrator privileges required.
	echo Right-click and select "Run as administrator".
	pause
	exit /b
)

cd /d %~dp0

set "INSTALLDIR=%APPDATA%\TheDreigon\Purple Polished Map"
if exist bin\Release\purple-polished-map.exe (
	set "SOURCEFILE=bin\Release\purple-polished-map.exe"
) else (
	set "SOURCEFILE=purple-polished-map.exe"
)
set "INSTALLPATH=%INSTALLDIR%\purple-polished-map.exe"

if not exist %SOURCEFILE% (
	echo purple-polished-map.exe does not exist.
	echo Download it to the same location as install.bat and retry.
	pause
	exit /b
)

mkdir "%INSTALLDIR%" >nul 2>&1
copy /b /y %SOURCEFILE% "%INSTALLPATH%" >nul

reg add "HKCR\.blk" /ve /f /d "Purple Polished Map" >nul
reg add "HKCR\Purple Polished Map" /ve /f /d "BLK Map File" >nul
rem reg add "HKCR\Purple Polished Map\DefaultIcon" /ve /f /d "%%SystemRoot%%\system32\imageres.dll,127" >nul
reg add "HKCR\Purple Polished Map\shell\open\command" /ve /f /d "\"%INSTALLPATH%\" \"%%1\"" >nul

set LNKSCRIPT="%TEMP%\purple-polished-map-lnk-%RANDOM%.vbs"
echo Set oWS = WScript.CreateObject("WScript.Shell") >> %LNKSCRIPT%
echo sLinkFile = "%USERPROFILE%\Desktop\Purple Polished Map.lnk" >> %LNKSCRIPT%
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> %LNKSCRIPT%
echo oLink.TargetPath = "%INSTALLPATH%" >> %LNKSCRIPT%
echo oLink.Save >> %LNKSCRIPT%
cscript /nologo %LNKSCRIPT%
del %LNKSCRIPT%

echo Installed Purple Polished Map to:
echo %INSTALLPATH%
echo Shortcut created on Desktop.
echo You may now delete the downloaded copy of purple-polished-map.exe.
pause
