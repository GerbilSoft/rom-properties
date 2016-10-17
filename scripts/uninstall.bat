@ECHO OFF
ECHO.
ECHO rom-properties shell extension uninstallation script
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

ECHO This batch file will unregister the ROM Properties DLL.
ECHO NOTE: Unregistering will disable all functionality provided
ECHO by the DLL for supported ROM files.
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

ECHO Unregistering rom-properties-amd64.dll...
REGSVR32 /S /U rom-properties-amd64.dll
IF ERRORLEVEL 1 (
	ECHO ERROR: 64-bit DLL unregistration failed.
	PAUSE
	EXIT /B 1
)
ECHO 64-bit DLL unregistration successful.
ECHO.

IF NOT EXIST rom-properties-i386.dll (
	ECHO WARNING: rom-properties-i386.dll not found.
	ECHO Skipping 32-bit DLL unregistration.
) ELSE (
	ECHO Unregistering rom-properties-i386.dll...
	IF ERRORLEVEL 1 (
		ECHO ERROR: 32-bit DLL unregistration failed.
	) ELSE (
		ECHO 32-bit DLL unregistration successful.
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

ECHO Unregistering rom-properties-i386.dll...
REGSVR32 /S /U rom-properties-i386.dll
IF ERRORLEVEL 1 (
	ECHO ERROR: 32-bit DLL unregistration failed.
	PAUSE
	EXIT /B 1
)
ECHO 32-bit DLL unregistration successful.
ECHO.
PAUSE
EXIT /B 0
