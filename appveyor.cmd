@ECHO OFF
cmake --version

:: Enable or disable optional features?
if "%1" == "OFF" (
	set OPTEN=OFF
) else (
	set OPTEN=ON
)
set "OPTFEAT=-DENABLE_EXTRA_SECURITY=%OPTEN% -DENABLE_JPEG=%OPTEN% -DENABLE_XML=%OPTEN% -DENABLE_DECRYPTION=%OPTEN% -DENABLE_UNICE68=%OPTEN% -DENABLE_LIBMSPACK=%OPTEN% -DENABLE_PVRTC=%OPTEN% -DENABLE_ZSTD=%OPTEN% -DENABLE_LZ4=%OPTEN% -DENABLE_LZO=%OPTEN% -DENABLE_NLS=%OPTEN%"

:: FIXME: Get multiple compilers working again.
:: For now, it's MSVC only for the Windows build.
goto :msvc2015

if "%compiler%" == "msvc2015" goto :msvc2015
if "%compiler%" == "mingw-w64" goto :mingw-w64
echo *** ERROR: Unsupported compiler '%compiler%'.
exit /b 1

:msvc2015
set PreferredToolArchitecture=x64
set "CMAKE_GENERATOR=Visual Studio 14 2015"
set CMAKE_GENERATOR_TOOLSET=v140_xp
if "%platform%" == "x64" set "CMAKE_GENERATOR=%CMAKE_GENERATOR% Win64"
mkdir build
cd build
cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_GENERATOR_TOOLSET=%CMAKE_GENERATOR_TOOLSET% -DBUILD_TESTING=ON -DENABLE_LTO=OFF -DENABLE_PCH=ON %OPTFEAT%
exit /b %ERRORLEVEL%

:mingw-w64
set PATH=%PATH:C:\Program Files\Git\bin;=%
set PATH=%PATH:C:\Program Files\Git\usr\bin;=%
set PATH=%PATH:C:\Program Files (x86)\Git\bin;=%
set PATH=%PATH:C:\Program Files (x86)\Git\usr\bin;=%
if "%platform%" == "x86" set MINGW64_ROOT=C:/mingw-w64/i686-6.3.0-posix-dwarf-rt_v5-rev1/mingw32
if "%platform%" == "x64" set MINGW64_ROOT=C:/mingw-w64/x86_64-7.2.0-posix-seh-rt_v5-rev1/mingw64
set "PATH=%MINGW64_ROOT%\bin;%PATH%"

mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_PREFIX_PATH=%MINGW64_ROOT% -DCMAKE_C_COMPILER=%MINGW64_ROOT%/bin/gcc.exe -DCMAKE_CXX_COMPILER=%MINGW64_ROOT%/bin/g++.exe -DCMAKE_BUILD_TYPE=%CONFIGURATION% -DBUILD_TESTING=ON -DENABLE_LTO=OFF -DENABLE_PCH=ON %OPTFEAT%
exit /b %ERRORLEVEL%
