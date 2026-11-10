@echo off

::%1第一个参数, 平台, Win32 or x64
::%2第二个参数, 编译选项, Debug or Release

echo Platform: "%1"
echo Configuration: "%2"
echo TargetOS: "%3"
echo EchoSDK: "%4"

set Platform=%1
set Configuration=%2
set TargetOS=%3
set EchoSDK=%4

IF "%1" == "" (
	echo ! ! ! ERROR ! ! !  The Platform is null!
	goto Exit
)
IF "%2" == "" (
	echo ! ! ! ERROR ! ! !  The Configuration is null!
	goto Exit
)
IF "%3" == "" (
	echo ! ! ! ERROR ! ! !  The TargetOS is null!
	goto Exit
)
IF "%4" == "" (
	echo ! ! ! ERROR ! ! !  The EchoSDK is null!
	goto Exit
)



set FMOD_Suffix=
IF "%1" == "Win32" (
	IF "%2" == "Debug" (
		SET FMOD_Suffix=L
	) ELSE IF "%2" == "Release" (
		SET FMOD_Suffix=
	)
) ELSE IF "%1" == "x64" (
	IF "%2" == "Debug" (
		SET FMOD_Suffix=L
	) ELSE IF "%2" == "Release" (
		SET FMOD_Suffix=
	) ELSE IF "%2%" == "Debug_Editor" (
		SET FMOD_Suffix=L
		SET Configuration=Debug
	) ELSE IF "%2%" == "Release_Editor" (
		SET FMOD_Suffix=
		SET Configuration=Release
	) ELSE IF "%2%" == "EditorDebug" (
		SET FMOD_Suffix=L
	) ELSE IF "%2%" == "EditorRelease" (
		SET FMOD_Suffix=
	)
)

set DestBinDir=..\..\Bin\%TargetOS%\%Configuration%
set EchoSDKBinDir=%EchoSDK%\Bin\%TargetOS%\%Platform%\%Configuration%

xcopy %EchoSDKBinDir%\*.dll			%DestBinDir%\	/y /d /f /i


xcopy %EchoSDKBinDir%\fmod.dll		%DestBinDir%\ /y /d /f /i
xcopy %EchoSDKBinDir%\fmodstudio.dll		%DestBinDir%\ /y /d /f /i

IF "%2" == "Debug" (
xcopy %EchoSDKBinDir%\fmod%FMOD_Suffix%.dll		%DestBinDir%\ /y /d /f /i
xcopy %EchoSDKBinDir%\fmodstudio%FMOD_Suffix%.dll		%DestBinDir%\ /y /d /f /i
) ELSE IF "%2" == "EditorDebug" (
xcopy %EchoSDKBinDir%\fmod%FMOD_Suffix%.dll		%DestBinDir%\ /y /d /f /i
xcopy %EchoSDKBinDir%\fmodstudio%FMOD_Suffix%.dll		%DestBinDir%\ /y /d /f /i
)


:Exit
