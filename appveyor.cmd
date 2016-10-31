@ECHO OFF
IF "%PLATFORM%" == "x64" (
	cmake . -G "Visual Studio 14 2015 Win64" -DBUILD_TESTING=ON
) ELSE (
	cmake . -G "Visual Studio 14 2015" -DBUILD_TESTING=ON
)
