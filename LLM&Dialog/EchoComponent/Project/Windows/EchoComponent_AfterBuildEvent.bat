@echo off

echo Platform: "%1"
echo Configuration: "%2"
echo ProjectName: "%3"
echo TargetOS: "%4"
echo EchoSDK: "%5"

IF "%1" == "" (
	echo ! ! ! ERROR ! ! !  The Platform is null!
	goto Exit
)
IF "%2" == "" (
	echo ! ! ! ERROR ! ! !  The Configuration is null!
	goto Exit
)
IF "%3" == "" (
	echo ! ! ! ERROR ! ! !  The ProjectName is null!
	goto Exit
)
IF "%4" == "" (
	echo ! ! ! ERROR ! ! !  The Target OS is null!
	goto Exit
)

xcopy "..\..\Lib\%4\%1\%2\%3.lib" "%5\Lib\%4\%1\%2\" /y /f
xcopy "..\..\Lib\%4\%1\%2\%3.dll" "%5\Bin\%4\%1\%2\" /y /f
xcopy "..\..\Lib\%4\%1\%2\%3.pdb" "%5\PDB\%4\%1\%2\" /y /f

:Exit
