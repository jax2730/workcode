@echo off

echo EchoSDK: "%1"

set DestDir=%1\Include\EchoComponent
set DestPrivateDir=%1\PrivateInclude\EchoComponent

IF EXIST %DestDir% (
	DEL %DestDir% /S /Q /F > nul
) ELSE (
	MD %DestDir%
)

xcopy ..\..\Include %DestDir%\ /s /q /y
xcopy ..\..\Script\Include %DestDir%\ /s /q /y
xcopy ..\..\PrivateInclude\*.h  %DestPrivateDir%\  /s /q /y