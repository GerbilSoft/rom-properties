@ECHO OFF
SETLOCAL
:: Packaging script for rom-properties, Windows version.
:: Requires the following:
:: - CMake 2.8.12 or later
:: - MSVC 2015 with 32-bit and 64-bit compilers
:: - Windows 7 SDK
:: - zip.exe and unzip.exe in %PATH%
::
:: NOTE: The test suite is NOT run here.

ECHO.
ECHO rom-properties packaging script for Windows
ECHO.

:: Check for zip.exe and unzip.exe.
FOR %%X IN (zip.exe) do (set FOUND=%%~$PATH:X)
IF NOT DEFINED FOUND (
	ECHO *** ERROR: zip.exe was not found in PATH.
	ECHO Please download Info-ZIP from http://www.info-zip.org/
	ECHO and place zip.exe and unzip.exe in PATH.
	PAUSE
	EXIT /B 1
)
FOR %%X IN (unzip.exe) do (set FOUND=%%~$PATH:X)
IF NOT DEFINED FOUND (
	ECHO *** ERROR: unzip.exe was not found in PATH.
	ECHO Please download Info-ZIP from http://www.info-zip.org/
	ECHO and place zip.exe and unzip.exe in PATH.
	PAUSE
	EXIT /B 1
)

:: Check for a file known to be in the top-level source directory.
IF NOT EXIST travis.sh (
	ECHO *** ERROR: This batch file must be run from the top-level
	ECHO source directory. Change to the top-level source directory
	ECHO and rerun this batch file by typing:
	ECHO.
	ECHO scripts\package.cmd
	ECHO.
	PAUSE
	EXIT /B 1
)

:: Clear ERRORLEVEL before doing anything else.
:: TODO: Show error messages.
CMD /C "EXIT /B 0"

:: Clear the packaging prefix.
ECHO Clearing pkg_windows directory...
RMDIR /S /Q pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
MKDIR pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
CHDIR pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Compile the 32-bit version.
ECHO.
ECHO Compiling 32-bit rom-properties-i386.dll...
MKDIR build.i386
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.i386
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio 14 2015" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DSPLIT_DEBUG=ON
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cpack -C Release
POPD

:: Compile the 64-bit version.
ECHO.
ECHO Compiling 64-bit rom-properties-amd64.dll...
MKDIR build.amd64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.amd64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio 14 2015 Win64" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DSPLIT_DEBUG=ON
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cpack -C Release
POPD

:: TODO: Merge the two packages.
EXIT /B 0
