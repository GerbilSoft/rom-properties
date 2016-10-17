@ECHO OFF
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
	ECHO ERROR: This batch file must be run as Administrator.
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

ECHO ERROR: This CPU architecture is not supported.
PAUSE
EXIT /B 1

:amd64
IF NOT EXIST rom-properties-amd64.dll (
	ECHO ERROR: rom-properties-amd64.dll not found.
	PAUSE
	EXIT /B 1
)

ECHO Registering rom-properties-amd64.dll for 64-bit applications...
REGSVR32 /S rom-properties-amd64.dll
IF ERRORLEVEL 1 (
	ECHO ERROR: 64-bit DLL registration failed.
	PAUSE
	EXIT /B 1
)
ECHO 64-bit DLL registration successful.
ECHO.

IF NOT EXIST rom-properties-i386.dll (
	ECHO WARNING: rom-properties-i386.dll not found.
	ECHO This DLL is required in order to support 32-bit applications.
) ELSE (
	ECHO Registering rom-properties-i386.dll for 32-bit applications...
	IF ERRORLEVEL 1 (
		ECHO ERROR: 32-bit DLL registration failed.
		ECHO This DLL is required in order to support 32-bit applications.
	) ELSE (
		ECHO 64-bit DLL registration successful.
	)
)
ECHO.
PAUSE
EXIT /B 0

:i386
IF NOT EXIST rom-properties-i386.dll (
	ECHO ERROR: rom-properties-i386.dll not found.
	PAUSE
	EXIT /B 1
)

ECHO Registering rom-properties-i386.dll for 32-bit applications...
REGSVR32 /S rom-properties-i386.dll
IF ERRORLEVEL 1 (
	ECHO ERROR: 32-bit DLL registration failed.
	PAUSE
	EXIT /B 1
)
ECHO 32-bit DLL registration successful.
ECHO.
PAUSE
EXIT /B 0
