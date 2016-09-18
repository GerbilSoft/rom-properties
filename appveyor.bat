@ECHO OFF
IF "%PLATFORM%" == "x64" (
	cmake . -G "Visual Studio 14 2015 Win64"
) ELSE (
	cmake . -G "Visual Studio 14 2015"
)
