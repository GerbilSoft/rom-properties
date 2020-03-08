@ECHO OFF
cmake --version

if "%compiler%" == "msvc2013" goto :msvc2013
if "%compiler%" == "mingw-w64" goto :mingw-w64
echo *** ERROR: Unsupported compiler '%compiler%'.
exit /b 1

:msvc2013
set PreferredToolArchitecture=x64
set "CMAKE_GENERATOR=Visual Studio 12 2013"
set CMAKE_GENERATOR_TOOLSET=v120_xp
if "%platform%" == "x64" set "CMAKE_GENERATOR=%CMAKE_GENERATOR% Win64"
mkdir build
cd build
cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_GENERATOR_TOOLSET=%CMAKE_GENERATOR_TOOLSET% -DENABLE_JPEG=ON -DBUILD_TESTING=ON -DENABLE_LTO=OFF -DENABLE_PCH=ON
exit /b %ERRORLEVEL%

:mingw-w64
set PATH=%PATH:C:\Program Files\Git\bin;=%
set PATH=%PATH:C:\Program Files\Git\usr\bin;=%
set PATH=%PATH:C:\Program Files (x86)\Git\bin;=%
set PATH=%PATH:C:\Program Files (x86)\Git\usr\bin;=%
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\mingw64 -DCMAKE_BUILD_TYPE=%configuration% -DENABLE_JPEG=ON -DBUILD_TESTING=ON -DENABLE_LTO=OFF -DENABLE_PCH=ON
exit /b %ERRORLEVEL%
