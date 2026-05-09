# Win32-specific CFLAGS/CXXFLAGS.

# Basic platform flags:
# - Enable strict type checking in the Windows headers.
# - Define WIN32_LEAN_AND_MEAN to reduce the number of Windows headers included.
# - Define NOMINMAX to disable the MIN() and MAX() macros.
ADD_DEFINITIONS(-DSTRICT -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# NOTE: This program only supports Unicode on Windows.
# No support for ANSI Windows, i.e. Win9x.
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

# Minimum Windows version for the SDK is Windows 7.
ADD_DEFINITIONS(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0601)

# Enable secure template overloads for C++.
# References:
# - MinGW's _mingw_secapi.h
# - https://learn.microsoft.com/en-us/cpp/c-runtime-library/secure-template-overloads?view=msvc-170
ADD_DEFINITIONS(-D_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES=1)
ADD_DEFINITIONS(-D_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY=1)
ADD_DEFINITIONS(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
ADD_DEFINITIONS(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1)
ADD_DEFINITIONS(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY=1)

# Compiler-specific Win32 flags.
IF(MSVC)
	INCLUDE(cmake/platform/win32-msvc.cmake)
ELSE(MSVC)
	INCLUDE(cmake/platform/win32-gcc.cmake)
ENDIF(MSVC)
