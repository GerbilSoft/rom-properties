# CMake Toolchain file for Windows 10 on ARM
# Requires MSVC 2019 with ARM toolchain and Windows 10 v1809 SDK.

# NOTE: Support for 32-bit ARM was dropped in Windows 11.
# The latest Windows 10 SDK available with MSVC 2022 is 10.0.19041.0.
# We need to specify the exact version because otherwise CMake picks
# the latest installed SDK, which won't work properly on most setups.

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR ARM)
SET(CMAKE_SYSTEM_VERSION 10.0.19041.0)
SET(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0.19041.0)

# Unset WindowsSDKVersion to prevent conflicts if run in a Visual Studio command prompt.
UNSET(ENV{WindowsSDKVersion})

# Set the SDK version here.
# NOTE: Requires CMake 3.27.
# Reference: https://cmake.org/cmake/help/latest/variable/CMAKE_GENERATOR_PLATFORM.html
CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
SET(CMAKE_GENERATOR_PLATFORM arm,version=${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})

# MSVC host binaries directory
SET(MSVC_HOST_BIN_DIR "$ENV{ProgramFiles}/Microsoft Visual Studio/2019")
FOREACH(_msvc_edition Enterprise Professional Community)
	IF(EXISTS "${MSVC_HOST_BIN_DIR}/${_msvc_edition}" AND IS_DIRECTORY "${MSVC_HOST_BIN_DIR}/${_msvc_edition}")
		SET(MSVC_HOST_BIN_DIR "${MSVC_HOST_BIN_DIR}/${_msvc_edition}")
		BREAK()
	ENDIF()
ENDFOREACH(_msvc_edition)
# TODO: Allow different Host architectures?
SET(MSVC_HOST_BIN_DIR "${MSVC_HOST_BIN_DIR}/VC/Tools/MSVC/14.29.30133/bin/Hostx64/arm")

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER	"${MSVC_HOST_BIN_DIR}/cl.exe")
SET(CMAKE_CXX_COMPILER	"${MSVC_HOST_BIN_DIR}/cl.exe")
# NOTE: rc.exe is part of the SDK, not the compiler toolchain.
#SET(CMAKE_RC_COMPILER	"${MSVC_HOST_BIN_DIR}/rc.exe")

# Disable warning C5105:
# C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um\winbase.h(9572,5): error C2220: the following warning is treated as an error [build.arm\extlib\zlib-ng\zlib.vcxproj]
#   (compiling source file 'extlib/zlib-ng/arch/arm/arm_features.c')
# C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um\winbase.h(9572,5): warning C5105: macro expansion producing 'defined' has undefined behavior [build.arm\extlib\zlib-ng\zlib.vcxproj]
#   (compiling source file '../../../../extlib/zlib-ng/arch/arm/arm_features.c')
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd5105")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd5105")
