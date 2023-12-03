/**
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

// rom-properties: adding this here because AppVeyor's MSVC 2015
// build system doesn't have versionhelpers.h for some reason.
// NOTE: Removing the WINAPI_FAMILY_PARTITION checks.

#pragma once

#include "RpWin32_sdk.h"

#ifdef __cplusplus
#  define VERSIONHELPERAPI inline bool
#else
#  define VERSIONHELPERAPI FORCEINLINE BOOL
#endif

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
/* gcc complains about missing field initializers. */
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

VERSIONHELPERAPI IsWindowsVersionOrGreater(WORD major, WORD minor, WORD servpack)
{
    OSVERSIONINFOEXW vi = {sizeof(vi),major,minor,0,0,{0},servpack};
    return VerifyVersionInfoW(&vi, VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR,
        VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0,
            VER_MAJORVERSION,VER_GREATER_EQUAL),
            VER_MINORVERSION,VER_GREATER_EQUAL),
            VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL));
}

VERSIONHELPERAPI IsWindowsXPOrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0);
}

VERSIONHELPERAPI IsWindowsXPSP1OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 1);
}

VERSIONHELPERAPI IsWindowsXPSP2OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 2);
}

VERSIONHELPERAPI IsWindowsXPSP3OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 3);
}

VERSIONHELPERAPI IsWindowsVistaOrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0);
}

VERSIONHELPERAPI IsWindowsVistaSP1OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 1);
}

VERSIONHELPERAPI IsWindowsVistaSP2OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 2);
}

VERSIONHELPERAPI IsWindows7OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 0);
}

VERSIONHELPERAPI IsWindows7SP1OrGreater(void) {
    return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 1);
}

VERSIONHELPERAPI IsWindows8OrGreater(void) {
    // FIXME: _WIN32_WINNT_WIN8 is missing when building with the Windows 7 SDK.
    // Defining it causes a lot of other things to break for some reason...
    //return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0);
    return IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0);
}

VERSIONHELPERAPI IsWindows8Point1OrGreater(void) {
    // FIXME: _WIN32_WINNT_WINBLUE is missing when building with the Windows 7 SDK.
    // Defining it causes a lot of other things to break for some reason...
    //return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0);
    return IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0);
}

VERSIONHELPERAPI IsWindowsThresholdOrGreater(void) {
    // FIXME: _WIN32_WINNT_WINTHRESHOLD is missing when building with MSVC 2022 for some reason.
    //return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINTHRESHOLD), LOBYTE(_WIN32_WINNT_WINTHRESHOLD), 0);
    return IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0);
}

VERSIONHELPERAPI IsWindows10OrGreater(void) {
    return IsWindowsThresholdOrGreater();
}

VERSIONHELPERAPI IsWindowsVersionOrGreater_BuildNumberCheck(WORD major, WORD minor, DWORD buildnumber)
{
    OSVERSIONINFOEXW vi = {sizeof(vi),major,minor,buildnumber,0,{0},0};
    return VerifyVersionInfoW(&vi, VER_MAJORVERSION|VER_MINORVERSION|VER_BUILDNUMBER,
        VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0,
            VER_MAJORVERSION,VER_GREATER_EQUAL),
            VER_MINORVERSION,VER_GREATER_EQUAL),
            VER_BUILDNUMBER, VER_GREATER_EQUAL));
}

VERSIONHELPERAPI IsWindows10Build21277OrGreater(void) {
    // Windows 10 Build 21277 on ARM added amd64 emulation.
    // https://blogs.windows.com/windows-insider/2020/12/10/introducing-x64-emulation-in-preview-for-windows-10-on-arm-pcs-to-the-windows-insider-program/
    IsWindowsVersionOrGreater_BuildNumberCheck(HIBYTE(0x0A00), LOBYTE(0x0A00), 21277);
}

VERSIONHELPERAPI IsWindows11OrGreater(void) {
    // Windows 11 shows up as "Windows 10 build 22000".
    IsWindowsVersionOrGreater_BuildNumberCheck(HIBYTE(0x0A00), LOBYTE(0x0A00), 22000);
}

VERSIONHELPERAPI IsWindowsServer(void) {
    OSVERSIONINFOEXW vi = {sizeof(vi),0,0,0,0,{0},0,0,0,VER_NT_WORKSTATION};
    return !VerifyVersionInfoW(&vi, VER_PRODUCT_TYPE, VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL));
}
