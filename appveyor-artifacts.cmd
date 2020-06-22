@ECHO OFF
if not "%compiler%" == "msvc2013" exit /b 0
if not "%configuration%" == "Release" exit /b 0

cpack --version
cpack -C "%configuration%"
if ERRORLEVEL 1 exit /b %ERRORLEVEL%
for %%z in (*.zip) do appveyor PushArtifact "%%z"
exit /b %ERRORLEVEL%
