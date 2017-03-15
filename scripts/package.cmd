@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
CD /D "%~dp0"

:: Packaging script for rom-properties, Windows version.
:: Requires the following:
:: - CMake 3.0.0 or later
:: - MSVC 2010, 2012, 2013, 2015, or 2017 with 32-bit and 64-bit compilers
:: - Windows 7 SDK
:: - zip.exe and unzip.exe in %PATH%
::
:: NOTE: The test suite is NOT run here.

ECHO.
ECHO rom-properties packaging script for Windows
ECHO.

:: Check for a file known to be in the top-level source directory.
IF NOT EXIST travis.sh (
	:: Maybe we're running from the scripts directory?
	CD ..
	IF NOT EXIST travis.sh (
		ECHO *** ERROR: This batch file must be run from either the
		ECHO top-level source directory or from the scripts directory.
		ECHO.
		ECHO Change to the top-level source directory and rerun this
		ECHO batch file by typing:
		ECHO.
		ECHO scripts\package.cmd
		ECHO.
		PAUSE
		EXIT /B 1
	)
)

:: Determine the 32-bit "Program Files" directory.
IF NOT "%ProgramFiles(x86)%" == "" (
	SET "PRGFILES=%ProgramFiles(x86)%"
) ELSE (
	SET "PRGFILES=%ProgramFiles%"
)

:: Check for supported MSVC versions.
SET MSVC_DIR=
SET MSVC_VERSION=
SET MSVC_YEAR=
SET CMAKE_GENERATOR=
SET CMAKE_TOOLSET=
IF EXIST "%PRGFILES%\Microsoft Visual Studio 10.0\VC\bin\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio 10.0"
	SET MSVC_VERSION=10.0
	SET MSVC_YEAR=2010
	SET "CMAKE_GENERATOR=10 2010"
	SET CMAKE_TOOLSET=v100
)
IF EXIST "%PRGFILES%\Microsoft Visual Studio 11.0\VC\bin\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio 11.0"
	SET MSVC_VERSION=11.0
	SET MSVC_YEAR=2012
	SET "CMAKE_GENERATOR=11 2012"
	SET CMAKE_TOOLSET=v110_xp
)
IF EXIST "%PRGFILES%\Microsoft Visual Studio 12.0\VC\bin\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio 12.0"
	SET MSVC_VERSION=12.0
	SET MSVC_YEAR=2013
	SET "CMAKE_GENERATOR=12 2013"
	SET CMAKE_TOOLSET=v120_xp
)
IF EXIST "%PRGFILES%\Microsoft Visual Studio 14.0\VC\bin\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio 14.0"
	SET MSVC_VERSION=14.0
	SET MSVC_YEAR=2015
	SET "CMAKE_GENERATOR=14 2015"
	SET CMAKE_TOOLSET=v140_xp
)
IF EXIST "%PRGFILES%\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.10.25017\bin\HostX86\x86\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.10.25017"
	SET MSVC_VERSION=14.1
	SET MSVC_YEAR=2017
	SET "CMAKE_GENERATOR=15 2017"
	SET CMAKE_TOOLSET=v141_xp
)
IF EXIST "%PRGFILES%\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.10.25017\bin\HostX86\x86\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.10.25017"
	SET MSVC_VERSION=14.1
	SET MSVC_YEAR=2017
	SET "CMAKE_GENERATOR=15 2017"
	SET CMAKE_TOOLSET=v141_xp
)
IF EXIST "%PRGFILES%\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.10.25017\bin\HostX86\x86\cl.exe" (
	SET "MSVC_DIR=%PRGFILES%\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.10.25017"
	SET MSVC_VERSION=14.1
	SET MSVC_YEAR=2017
	SET "CMAKE_GENERATOR=15 2017"
	SET CMAKE_TOOLSET=v141_xp
)

IF "%CMAKE_GENERATOR%" == "" (
	ECHO *** ERROR: Supported version of MSVC was not found.
	ECHO Supported versions: 2010, 2012, 2013, 2015
	PAUSE
	EXIT /B 1
)
ECHO Using MSVC %MSVC_YEAR% (%MSVC_VERSION%) for packaging.
ECHO.

:: MSVC 2017 uses a different directory layout.
:: NOTE: This must be set here, since you can't use a variable
:: set in a block within the same block. (It'll have the previous
:: value for some reason.)
IF "%MSVC_YEAR%" == "2017" (
	SET "MSVC_CL=%MSVC_DIR%\bin\HostX86\x86\cl.exe"
	SET "MSVC_CL64_CROSS=%MSVC_DIR%\bin\HostX86\x64\cl.exe"
	SET "MSVC_CL64_NATIVE=%MSVC_DIR%\bin\HostX64\x64\cl.exe"
) ELSE (
	SET "MSVC_CL=%MSVC_DIR%\VC\bin\cl.exe"
	SET "MSVC_CL64_CROSS=%MSVC_DIR%\VC\bin\x86_amd64\cl.exe"
	SET "MSVC_CL64_NATIVE=%MSVC_DIR%\VC\bin\amd64\cl.exe"
)

:: Check for the 32-bit compiler.
IF NOT EXIST "%MSVC_CL%" (
	ECHO "%MSVC_CL%"
	ECHO *** ERROR: 32-bit cl.exe was not found.
	ECHO Please reinstall MSVC.
	PAUSE
	EXIT /B 1
)

:: Check for the 64-bit compiler.
:: (either cross-compiler and native compiler)
IF NOT EXIST "%MSVC_CL64_CROSS%" (
	IF NOT EXIST "%MSVC_CL64_NATIVE%" (
		ECHO *** ERROR: 64-bit cl.exe was not found.
		ECHO Please reinstall MSVC.
		PAUSE
		EXIT /B 1
	)
)

:: Check for the Windows 7 SDK. (either v7.1A or v7.0A)
IF NOT EXIST "%PRGFILES%\Microsoft SDKs\Windows\v7.1A\Include\Windows.h" (
	IF NOT EXIST "%PRGFILES%\Microsoft SDKs\Windows\v7.0A\Include\Windows.h" (
		ECHO *** ERROR: Windows 7 SDK was not found.
		ECHO Please install the Windows 7 SDK.
		PAUSE
		EXIT /B 1
	)
)

:: Check for cmake.exe.
FOR %%X IN (cmake.exe) DO (SET FOUND=%%~$PATH:X)
IF NOT DEFINED FOUND (
	ECHO *** ERROR: CMake was not found in PATH.
	ECHO Please download CMake from https://cmake.org/download/
	ECHO and make sure it's added to PATH.
	PAUSE
	EXIT /B 1
)

:: Check for zip.exe and unzip.exe.
SET FOUND=
FOR %%X IN (zip.exe) DO (SET FOUND=%%~$PATH:X)
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

:: Clear ERRORLEVEL before doing anything else.
:: TODO: Show error messages.
CMD /C "EXIT /B 0"

:: Clear the packaging prefix.
ECHO Clearing the pkg_windows directory...
RMDIR /S /Q pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
MKDIR pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
CHDIR pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Compile the 32-bit version.
ECHO.
ECHO Compiling 32-bit rom-properties.dll...
MKDIR build.i386
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.i386
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE_GENERATOR%" -DCMAKE_GENERATOR_TOOLSET=%CMAKE_TOOLSET% -DCMAKE_BUILD_TYPE=Release -DENABLE_JPEG=ON -DBUILD_TESTING=OFF -DSPLIT_DEBUG=ON
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cpack -C Release
POPD

:: Compile the 64-bit version.
ECHO.
ECHO Compiling 64-bit rom-properties.dll...
MKDIR build.amd64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.amd64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE_GENERATOR% Win64" -DCMAKE_GENERATOR_TOOLSET=%CMAKE_TOOLSET% -DCMAKE_BUILD_TYPE=Release -DENABLE_JPEG=ON -DBUILD_TESTING=OFF -DSPLIT_DEBUG=ON
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cpack -C Release
POPD

:: Merge the ZIP files.
ECHO.
ECHO Creating the distribution ZIP files...
MKDIR combined
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD combined
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Get the basename of the 32-bit ZIP file.
FOR %%A IN (..\build.i386\*-win32.zip) do (set ZIP_PREFIX=%%~nxA)
SET "ZIP_PREFIX=%ZIP_PREFIX:~0,-10%"
IF "%ZIP_PREFIX%" == "" (
	ECHO *** ERROR: The 32-bit ZIP file was not found.
	ECHO Something failed in the initial packaging process...
	PAUSE
	EXIT /B 1
)

:: Extract the 32-bit ZIP file.
unzip ..\build.i386\*-win32.zip
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Extract the 64-bit ZIP file.
:: (Only the architecture-specific directory.)
unzip ..\build.amd64\*-win64.zip amd64/*
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Compress the debug files.
DEL /q "..\..\%ZIP_PREFIX%-windows.debug.zip" >NUL 2>&1
zip "..\..\%ZIP_PREFIX%-windows.debug.zip" i386/*.pdb amd64/*.pdb
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Compress everything else.
DEL /q "..\..\%ZIP_PREFIX%-windows.zip" >NUL 2>&1
zip -r "..\..\%ZIP_PREFIX%-windows.zip" * -x *.pdb
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:: Package merged.
ECHO.
ECHO *** Windows packages created. ***
ECHO.
ECHO The following files have been created in the top-level
ECHO source directory:
ECHO - %ZIP_PREFIX%-windows.zip: Standard distribution.
ECHO - %ZIP_PREFIX%-windows.debug.zip: PDB files.
ECHO.
PAUSE
EXIT /B 0
