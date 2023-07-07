@ECHO OFF
if not "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2015" exit /b 0
if not "%CONFIGURATION%" == "Release" exit /b 0

cpack --version
cpack -C "%CONFIGURATION%"
if ERRORLEVEL 1 exit /b %ERRORLEVEL%
for %%z in (*.zip) do appveyor PushArtifact "%%z"
exit /b %ERRORLEVEL%
