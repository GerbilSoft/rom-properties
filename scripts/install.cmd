@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
CD /D "%~dp0"

ECHO.
ECHO rom-properties shell extension installation script
ECHO.
IF /I "%PROCESSOR_ARCHITEW6432%" == "" (
	ECHO PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
) ELSE (
	ECHO PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
	ECHO PROCESSOR_ARCHITEW6432=%PROCESSOR_ARCHITEW6432%
)
ECHO.

:: Check for administrator permission.
:: "net session" returns an error if not administrator.
NET SESSION >NUL 2>&1
IF ERRORLEVEL 1 (
	ECHO *** ERROR: This batch file must be run as Administrator.
	ECHO Right-click the batch file and select "Run as Administrator".
	PAUSE
	EXIT /B 1
)

ECHO This batch file will register the ROM Properties DLL with the
ECHO system, which will provide extra functionality for supported
ECHO ROM files in Windows Explorer.
ECHO.
ECHO Note that the DLL locations are hard-coded in the registry.
ECHO If you move the DLLs, you will have to rerun this script.
ECHO In addition, the DLLs will usually be locked by Explorer,
ECHO so you will need to run the uninstall script and restart
ECHO Windows Explorer in order to move the DLLs.
ECHO.
PAUSE
ECHO.

IF /I "%PROCESSOR_ARCHITECTURE%" == "AMD64" GOTO :amd64
IF /I "%PROCESSOR_ARCHITEW6432%" == "AMD64" GOTO :amd64
IF /I "%PROCESSOR_ARCHITECTURE%" == "x86" GOTO :i386

ECHO *** ERROR: This CPU architecture is not supported.
PAUSE
EXIT /B 1

:amd64

IF /I "%PROCESSOR_ARCHITEW6432%" == "AMD64" (
	:: Running in 32-bit cmd.
	SET "REGSVR32_32BIT=%SYSTEMROOT%\SYSTEM32\REGSVR32.EXE"
	SET "REGSVR32_64BIT=%SYSTEMROOT%\SYSNATIVE\REGSVR32.EXE"

	:: Check for the MSVC 2015 runtime.
	IF NOT EXIST "%SYSTEMROOT%\SYSTEM32\MSVCP140.DLL" GOTO :no_msvcrt
	IF NOT EXIST "%SYSTEMROOT%\SYSNATIVE\MSVCP140.DLL" GOTO :no_msvcrt
) ELSE (
	:: Running in 64-bit cmd.
	SET "REGSVR32_32BIT=%SYSTEMROOT%\SYSWOW64\REGSVR32.EXE"
	SET "REGSVR32_64BIT=%SYSTEMROOT%\SYSTEM32\REGSVR32.EXE"

	:: Check for the MSVC 2015 runtime.
	IF NOT EXIST "%SYSTEMROOT%\SYSWOW64\MSVCP140.DLL" GOTO :no_msvcrt
	IF NOT EXIST "%SYSTEMROOT%\SYSTEM32\MSVCP140.DLL" GOTO :no_msvcrt
)

IF NOT EXIST amd64\rom-properties.dll (
	ECHO *** ERROR: amd64\rom-properties.dll not found.
	PAUSE
	EXIT /B 1
)

ECHO Registering 64-bit rom-properties.dll...
"%REGSVR32_64BIT%" /S amd64\rom-properties.dll
IF ERRORLEVEL 1 (
	ECHO *** ERROR: 64-bit DLL registration failed.
	PAUSE
	EXIT /B 1
)
ECHO 64-bit DLL registration successful.
ECHO.

IF NOT EXIST i386\rom-properties.dll (
	ECHO *** WARNING: i386\rom-properties.dll not found.
	ECHO This DLL is required in order to support 32-bit applications.
) ELSE (
	ECHO Registering 32-bit rom-properties.dll for 32-bit applications...
	"%REGSVR32_32BIT%" /S i386\rom-properties.dll
	IF ERRORLEVEL 1 (
		ECHO *** ERROR: 32-bit DLL registration failed.
		ECHO This DLL is required in order to support 32-bit applications.
	) ELSE (
		ECHO 32-bit DLL registration successful.
	)
)
ECHO.
PAUSE
EXIT /B 0

:i386
SET "REGSVR32_32BIT=%SYSTEMROOT%\SYSTEM32\REGSVR32.EXE"

:: Check for the MSVC 2015 runtime.
IF NOT EXIST "%SYSTEMROOT%\SYSTEM32\MSVCP140.DLL" GOTO :no_msvcrt

IF NOT EXIST i386\rom-properties.dll (
	ECHO *** ERROR: i386\rom-properties.dll not found.
	PAUSE
	EXIT /B 1
)

ECHO Registering 32-bit rom-properties.dll for 32-bit applications...
"%REGSVR32_32BIT%" /S i386\rom-properties.dll
IF ERRORLEVEL 1 (
	ECHO *** ERROR: 32-bit DLL registration failed.
	PAUSE
	EXIT /B 1
)
ECHO 32-bit DLL registration successful.
ECHO.
PAUSE
EXIT /B 0

:no_msvcrt
ECHO *** ERROR: MSVC 2015 runtime was not found.
ECHO Please download and install the MSVC 2015 runtime packages from:
ECHO https://www.microsoft.com/en-us/download/details.aspx?id=53587
ECHO.
ECHO NOTE: On 64-bit systems, you will need to install both the 32-bit
ECHO and the 64-bit versions.
ECHO.
PAUSE
EXIT /B 1
