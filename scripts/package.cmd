@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
CD /D "%~dp0\.."

:: Packaging script for rom-properties, Windows version.
:: Requires the following:
:: - CMake 3.0.0 or later
:: - MSVC 2015, or 2017 with 32-bit and 64-bit compilers
:: - MSVC 2019, 2022 with v141_xp toolchain is also supported.
:: - Windows 7 SDK
:: - zip.exe and unzip.exe in %PATH%
:: - Inno Setup 5 (for Windows XP/2003/Vista installation)
:: - Inno Setup 6 (for Windows 7/8/10/11 installation)
::
:: For ARM and ARM64 targets, MSVC 2019 is required.
:: For the ARM64EC target, MSVC 2022 and CMake 3.22.0 are required.
::
:: NOTE: The test suite is NOT run here.

ECHO.
ECHO rom-properties packaging script for Windows
ECHO.

:: Determine the 32-bit "Program Files" directory.
IF NOT "%ProgramFiles(x86)%" == "" (
	SET "PrgFiles32=%ProgramFiles(x86)%"
) ELSE (
	SET "PrgFiles32=%ProgramFiles%"
)

:: Determine the 64-bit "Program Files" directory.
IF NOT "%ProgramW6432%" == "" (
	SET "PrgFiles64=%ProgramW6432%"
) ELSE (
	SET "PrgFiles64=%ProgramFiles%"
)

SET MSVC32_DIR=
SET MSVC32_VERSION=
SET MSVC32_YEAR=
SET CMAKE32_GENERATOR=
SET CMAKE32_TOOLSET=
SET CMAKE32_ARCH=

SET MSVC64_DIR=
SET MSVC64_VERSION=
SET MSVC64_YEAR=
SET CMAKE64_GENERATOR=
SET CMAKE64_TOOLSET=
SET CMAKE64_ARCH=

:: Check for supported MSVC versions.
:: MSVC 2015
FOR %%I IN (2015.14) DO (
	SET J=%%I
	SET YEAR=!J:~0,4!
	SET V=!J:~5,2!
	IF EXIST "%PrgFiles32%\Microsoft Visual Studio !V!.0\VC\bin\cl.exe" (
		SET "MSVC32_DIR=%PrgFiles32%\Microsoft Visual Studio !V!.0"
		SET MSVC32_VERSION=!V!.0
		SET MSVC32_YEAR=!YEAR!
		SET CMAKE32_GENERATOR=!V! !YEAR!
		SET CMAKE32_TOOLSET=v!V!0_xp

		SET "MSVC64_DIR=%PrgFiles32%\Microsoft Visual Studio !V!.0"
		SET MSVC64_VERSION=!V!.0
		SET MSVC64_YEAR=!YEAR!
		SET CMAKE64_GENERATOR=!V! !YEAR! Win64
		SET CMAKE64_TOOLSET=v!V!0
	)
)

:: MSVC 2017
FOR %%I IN (Community Professional Enterprise) DO (
	IF EXIST "%PrgFiles32%\Microsoft Visual Studio\2017\%%I\VC\Tools\MSVC\14.16.27023\bin\HostX86\x86\cl.exe" (
		SET "MSVC32_DIR=%PrgFiles32%\Microsoft Visual Studio\2017\%%I\VC\Tools\MSVC\14.16.27023"
		SET MSVC32_VERSION=14.16
		SET MSVC32_YEAR=2017
		SET CMAKE32_GENERATOR=15 2017
		SET CMAKE32_TOOLSET=v141_xp

		SET "MSVC64_DIR=%PrgFiles32%\Microsoft Visual Studio\2017\%%I\VC\Tools\MSVC\14.16.27023"
		SET MSVC64_VERSION=14.16
		SET MSVC64_YEAR=2017
		SET CMAKE64_GENERATOR=15 2017 Win64
		SET CMAKE64_TOOLSET=v141
	)
)

:: MSVC 2019: Use the 2017 (14.1x) compiler for 32-bit in order to maintain WinXP compatibilty.
FOR %%I IN (Community Professional Enterprise) DO (
	IF EXIST "%PrgFiles32%\Microsoft Visual Studio\2019\%%I\VC\Tools\MSVC\14.29.30133\bin\HostX86\x86\cl.exe" (
		SET "MSVC32_DIR=%PrgFiles32%\Microsoft Visual Studio\2019\%%I\VC\Tools\MSVC\14.16.27023"
		SET MSVC32_VERSION=14.16
		SET MSVC32_YEAR=2019
		SET CMAKE32_GENERATOR=16 2019
		SET CMAKE32_TOOLSET=v141_xp
		SET CMAKE32_ARCH=-A Win32

		SET "MSVC64_DIR=%PrgFiles32%\Microsoft Visual Studio\2019\%%I\VC\Tools\MSVC\14.29.30133"
		SET MSVC64_VERSION=14.29
		SET MSVC64_YEAR=2019
		SET CMAKE64_GENERATOR=16 2019
		SET CMAKE64_TOOLSET=v142
		SET CMAKE64_ARCH=-A x64
	)
)

:: MSVC 2022: Use the 2017 (14.1x) compiler for 32-bit in order to maintain WinXP compatibilty.
:: NOTE: MSVC 2022 switched to 64-bit Program Files
FOR %%I IN (Community Professional Enterprise) DO (
	IF EXIST "%PrgFiles64%\Microsoft Visual Studio\2022\%%I\VC\Tools\MSVC\14.36.32532\bin\HostX86\x86\cl.exe" (
		SET "MSVC32_DIR=%PrgFiles64%\Microsoft Visual Studio\2022\%%I\VC\Tools\MSVC\14.16.27023"
		SET MSVC32_VERSION=14.16
		SET MSVC32_YEAR=2022
		SET CMAKE32_GENERATOR=17 2022
		SET CMAKE32_TOOLSET=v141_xp
		SET CMAKE32_ARCH=-A Win32

		SET "MSVC64_DIR=%PrgFiles64%\Microsoft Visual Studio\2022\%%I\VC\Tools\MSVC\14.36.32532"
		SET MSVC64_VERSION=14.36
		SET MSVC64_YEAR=2022
		SET CMAKE64_GENERATOR=17 2022
		SET CMAKE64_TOOLSET=v143
		SET CMAKE64_ARCH=-A x64
	)
)

IF "%CMAKE64_GENERATOR%" == "" (
	ECHO *** ERROR: Supported version of MSVC was not found.
	ECHO Supported versions: 2022, 2019, 2017, 2015
	PAUSE
	EXIT /B 1
)

:: Check for Inno Setup.
SET "ISCC5=%PrgFiles32%\Inno Setup 5\ISCC.exe"
IF NOT EXIST "%ISCC5%" (
	ECHO *** ERROR: Inno Setup 5 was not found.
	ECHO Get it at: https://jrsoftware.org/isdlold.php#5
	PAUSE
	EXIT /B 1
)
SET "ISCC6=%PrgFiles32%\Inno Setup 6\ISCC.exe"
IF NOT EXIST "%ISCC6%" (
	ECHO *** ERROR: Inno Setup 6 was not found.
	ECHO Get it at: https://jrsoftware.org/isdl.php
	PAUSE
	EXIT /B 1
)

ECHO Using the following MSVC versions for packaging:
ECHO - i386:    MSVC %MSVC32_YEAR% (%MSVC32_VERSION%)
ECHO - amd64:   MSVC %MSVC64_YEAR% (%MSVC64_VERSION%)
SET MSG_ARM=not building; requires MSVC 2019 or later
SET MSG_ARM64EC=not building; requires MSVC 2022 or later
IF "%MSVC64_YEAR%" == "2019" SET MSG_ARM=MSVC %MSVC64_YEAR% (%MSVC64_VERSION%)
IF "%MSVC64_YEAR%" == "2022" SET MSG_ARM=MSVC %MSVC64_YEAR% (%MSVC64_VERSION%)
IF "%MSVC64_YEAR%" == "2022" SET MSG_ARM64EC=MSVC %MSVC64_YEAR% (%MSVC64_VERSION%)
ECHO - arm:     %MSG_ARM%
ECHO - arm64:   %MSG_ARM%
ECHO - arm64ec: %MSG_ARM64EC%
ECHO.

:: MSVC 2017+ uses a different directory layout.
SET MSVC_CL=
FOR %%I IN (2017 2019 2022) DO (
	IF "%MSVC64_YEAR%" == "%%I" (
		SET "MSVC_CL=%MSVC32_DIR%\bin\HostX86\x86\cl.exe"
		SET "MSVC_CL64_CROSS=%MSVC64_DIR%\bin\HostX86\x64\cl.exe"
		SET "MSVC_CL64_NATIVE=%MSVC64_DIR%\bin\HostX64\x64\cl.exe"
	)
)
IF "%MSVC_CL%" == "" (
	SET "MSVC_CL=%MSVC32_DIR%\VC\bin\cl.exe"
	SET "MSVC_CL64_CROSS=%MSVC64_DIR%\VC\bin\x86_amd64\cl.exe"
	SET "MSVC_CL64_NATIVE=%MSVC64_DIR%\VC\bin\amd64\cl.exe"
)

:: Check for the 32-bit compiler.
IF NOT EXIST "%MSVC_CL%" (
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
IF NOT EXIST "%PrgFiles32%\Microsoft SDKs\Windows\v7.1A\Include\Windows.h" (
	IF NOT EXIST "%PrgFiles32%\Microsoft SDKs\Windows\v7.0A\Include\Windows.h" (
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
FOR %%X IN (unzip.exe) DO (SET FOUND=%%~$PATH:X)
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

:: PDB files
SET "PDB_FILES=fmt-12.pdb libpng16.pdb lz4.pdb minilzo.pdb minizip.pdb pugixml.pdb romdata-8.pdb rom-properties.pdb rpcli.pdb rp-config.pdb rp-download.pdb zlib1.pdb"

:: Clear the packaging prefix.
ECHO Clearing the pkg_windows directory...
MKDIR pkg_windows 2>NUL
CHDIR pkg_windows
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
FOR /D %%A IN (*) DO (RMDIR /S /Q "%%A")

:: Compile the i386 version.
ECHO.
ECHO Compiling i386 (32-bit) rom-properties.dll...
MKDIR build.i386
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.i386
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE32_GENERATOR%" %CMAKE32_ARCH% ^
	-DCMAKE_GENERATOR_TOOLSET=%CMAKE32_TOOLSET% ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DBUILD_TESTING=OFF ^
	-DSPLIT_DEBUG=ON
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
:: Copy the .pdb files.
MKDIR ..\pdb
MKDIR ..\pdb\i386
FOR %%A IN (%PDB_FILES%) DO (COPY /Y "bin\Release\%%A" ..\pdb\i386)
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
POPD

:: Compile the amd64 version.
ECHO.
ECHO Compiling amd64 (64-bit) rom-properties.dll...
MKDIR build.amd64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.amd64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE64_GENERATOR%" %CMAKE64_ARCH% ^
	-DCMAKE_GENERATOR_TOOLSET=%CMAKE64_TOOLSET% ^
	-DCMAKE_SYSTEM_VERSION=10.0 ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DBUILD_TESTING=OFF ^
	-DSPLIT_DEBUG=ON
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
MKDIR ..\pdb\amd64
FOR %%A IN (%PDB_FILES%) DO (COPY /Y "bin\Release\%%A" ..\pdb\amd64)
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
POPD

:: If not MSVC 2019 or later, skip ARM.
IF "%MSVC64_YEAR%" == "2019" GOTO :buildARM
IF "%MSVC64_YEAR%" == "2022" GOTO :buildARM
ECHO *** WARNING: MSVC 2019 or later is required for ARM builds.
ECHO *** Skipping ARM builds.
GOTO :doPackage

:buildARM
:: FIXME: Need to build gettext for ARM, ARM64, and ARM64EC.
:: May need to use the MSVC port for ARM64EC.
:: For now, disabling NLS on the ARM builds.

:: NOTE: Since we're cross-compiling, the i386 build will be used
:: for amiiboc.exe.
:: FIXME: It's not accepting "../build.i386"; using %~dp0 instead.

:: Compile the arm version.
SET BUILD_arm=1
ECHO.
ECHO Compiling arm (32-bit) rom-properties.dll...
MKDIR build.arm
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.arm
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE64_GENERATOR%" -A arm ^
	-DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain/arm-msvc-%MSVC64_YEAR%-win10-1809.cmake ^
	-DCMAKE_SYSTEM_VERSION=10.0 ^
	-DIMPORT_EXECUTABLES=%~dp0..\pkg_windows\build.i386 ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DBUILD_TESTING=OFF ^
	-DSPLIT_DEBUG=ON ^
	-DENABLE_NLS=OFF
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
MKDIR ..\pdb\arm
FOR %%A IN (%PDB_FILES%) DO (COPY /Y "bin\Release\%%A" ..\pdb\arm)
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
POPD

:: Compile the arm64 version.
SET BUILD_arm64=1
ECHO.
ECHO Compiling arm64 (64-bit) rom-properties.dll...
MKDIR build.arm64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.arm64
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE64_GENERATOR%" -A arm64 ^
	-DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain/arm64-msvc-%MSVC64_YEAR%-win10-1809.cmake ^
	-DCMAKE_SYSTEM_VERSION=10.0 ^
	-DIMPORT_EXECUTABLES=%~dp0..\pkg_windows\build.i386 ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DBUILD_TESTING=OFF ^
	-DSPLIT_DEBUG=ON ^
	-DENABLE_NLS=OFF
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
MKDIR ..\pdb\arm64
FOR %%A IN (%PDB_FILES%) DO (COPY /Y "bin\Release\%%A" ..\pdb\arm64)
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
POPD

IF "%MSVC64_YEAR%" == "2022" GOTO :buildARM64EC
ECHO *** WARNING: MSVC 2022 or later is required for ARM64EC builds.
ECHO *** Skipping ARM64EC builds.
GOTO :doPackage

:buildARM64EC
:: Compile the arm64ec version.
SET BUILD_arm64ec=1
ECHO.
ECHO Compiling arm64ec (64-bit Emulation Compatible) rom-properties.dll...
MKDIR build.arm64ec
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
PUSHD build.arm64ec
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake ..\.. -G "Visual Studio %CMAKE64_GENERATOR%" -A arm64ec ^
	-DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain/arm64-msvc-%MSVC64_YEAR%-win10-1809.cmake ^
	-DCMAKE_SYSTEM_VERSION=10.0 ^
	-DIMPORT_EXECUTABLES=%~dp0..\pkg_windows\build.i386 ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DBUILD_TESTING=OFF ^
	-DSPLIT_DEBUG=ON ^
	-DENABLE_NLS=OFF
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
cmake --build . --config Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
MKDIR ..\pdb\arm64ec
FOR %%A IN (%PDB_FILES%) DO (COPY /Y "bin\Release\%%A" ..\pdb\arm64ec)
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
POPD

:doPackage
:: FIXME: setup6.iss expects the arm/arm64/arm64ec DLLs to be available.
:: Need to make them optional if not building for ARM?
CD /D "%~dp0"
"%ISCC5%" setup5.iss /O..
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
"%ISCC6%" setup6.iss /O..
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

:doCombine
:: Compress the debug files.
:: TODO: Get the version number? (We're not using cpack anymore...)
SET "ZIP_PREFIX=rom-properties"
PUSHD ..\pkg_windows\pdb
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
DEL /Q "..\..\%ZIP_PREFIX%-windows.debug.zip" >NUL 2>&1
zip "..\..\%ZIP_PREFIX%-windows.debug.zip" */*.pdb
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
POPD

:: Packages created.
ECHO.
ECHO *** Windows packages created. ***
ECHO.
ECHO The following files have been created in the top-level source directory:
ECHO - SetupRomProperties.exe: Installer for Windows 7/8/10/11
ECHO - SetupRomPropertiesXP.exe: Installer for Windows XP/2003/Vista
ECHO - %ZIP_PREFIX%-windows.debug.zip: PDB files
ECHO.
PAUSE
EXIT /B 0
