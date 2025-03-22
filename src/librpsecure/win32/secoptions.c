/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure/win32)                *
 * secoptions.c: Security options for executables.                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: Using LocalAlloc()/LocalFree() here to prevent issues
// mixing and matching static and dynamic CRT versions.

#include "secoptions.h"

// C includes
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// Windows includes
#include <windows.h>
#include <sdkddkver.h>
#include <winternl.h>
#include <tchar.h>
#include "libwin32common/rp_versionhelpers.h"

#ifndef _WIN64

// NtSetInformationProcess() (needed for DEP on XP SP2)
typedef NTSTATUS (WINAPI *pfnNTSetInformationProcess_t)(
	HANDLE ProcessHandle,
	_In_ PROCESSINFOCLASS ProcessInformationClass,
	_In_ PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength);
#ifndef MEM_EXECUTE_OPTION_DISABLE
#  define MEM_EXECUTE_OPTION_DISABLE 2
#endif
#ifndef MEM_EXECUTE_OPTION_ATL7_THUNK_EMULATION
#  define MEM_EXECUTE_OPTION_ATL7_THUNK_EMULATION 4
#endif
#ifndef MEM_EXECUTE_OPTION_PERMANENT
#  define MEM_EXECUTE_OPTION_PERMANENT 8
#endif
// NOTE: ProcessExecuteFlags might be defined in an enum in newer
// versions of the Windows SDK and/or MinGW-w64. We'll use a macro
// instead of an enum value.
#ifndef ProcessExecuteFlags
#  define ProcessExecuteFlags 0x22
#endif

// DEP policy. (Vista SP1; later backported to XP SP3)
typedef BOOL (WINAPI *pfnSetProcessDEPPolicy_t)(_In_ DWORD dwFlags);
#ifndef PROCESS_DEP_ENABLE
#  define PROCESS_DEP_ENABLE 0x1
#endif
#ifndef PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION
#  define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION 0x2
#endif

#endif /* _WIN64 */

// SetProcessMitigationPolicy (Win8)
// Reference: https://git.videolan.org/?p=vlc/vlc-2.2.git;a=commitdiff;h=054cf24557164f79045d773efe7da87c4fe357de;hp=52e4b740ad47574bdff7b80aba4949311e1b88f1
#include "secoptions_win8.h"
typedef BOOL (WINAPI *pfnSetProcessMitigationPolicy_t)(_In_ PROCESS_MITIGATION_POLICY MitigationPolicy, _In_ PVOID lpBuffer, _In_ SIZE_T dwLength);

/**
 * Harden the process's integrity level policy.
 *
 * References:
 * - https://github.com/chromium/chromium/blob/4e88a3c4fa53bf4d3622d07fd13f3812d835e40f/sandbox/win/src/restricted_token_utils.cc
 * - https://github.com/chromium/chromium/blob/master/sandbox/win/src/restricted_token_utils.cc
 *
 * @return 0 on success; GetLastError() on error.
 */
static DWORD HardenProcessIntegrityLevelPolicy(void)
{
	DWORD dwLastError;
	DWORD nLengthNeeded;
	HANDLE hToken;
	char *pSecurityDescriptor = NULL;

	PACL sacl = NULL;
	BOOL sacl_present = FALSE;
	BOOL sacl_defaulted = FALSE;
	DWORD dwAceIndex;

	if (!OpenProcessToken(
		GetCurrentProcess(),		// ProcessHandle
		READ_CONTROL | WRITE_OWNER,	// DesiredAccess
		&hToken))			// TokenHandle
	{
		// OpenProcessToken() failed.
		return GetLastError();
	}

	// Get the object's security descriptor.
	/** BEGIN: Chromium's GetObjectSecurityDescriptor() **/
	GetKernelObjectSecurity(
		hToken,				// Handle
		LABEL_SECURITY_INFORMATION,	// RequestedInformation
		NULL,				// pSecurityDescriptor
		0,				// nLength
		&nLengthNeeded);		// lpnLengthNeeded
	dwLastError = GetLastError();
	if (dwLastError != ERROR_INSUFFICIENT_BUFFER) {
		// An unexpected error occurred.
		return dwLastError;
	}

	pSecurityDescriptor = LocalAlloc(LMEM_FIXED, nLengthNeeded);
	assert(pSecurityDescriptor != NULL);
	if (!pSecurityDescriptor) {
		// LocalAlloc() failed.
		dwLastError = ERROR_NOT_ENOUGH_MEMORY;
		goto out;
	}

	if (!GetKernelObjectSecurity(
		hToken,				// Handle
		LABEL_SECURITY_INFORMATION,	// RequestedInformation
		pSecurityDescriptor,		// pSecurityDescriptor
		nLengthNeeded,			// nLength
		&nLengthNeeded))		// lpnLengthNeeded
	{
		// Failed to get the security descriptor.
		dwLastError = GetLastError();
		goto out;
	}

	/** END: Chromium's GetObjectSecurityDescriptor() **/

	// Get the System Access Control List.
	if (!GetSecurityDescriptorSacl(
		pSecurityDescriptor,	// pSecurityDescriptor
		&sacl_present,		// lpbSaclPresent
		&sacl,			// pSacl
		&sacl_defaulted))	// lpbSaclDefaulted
	{
		// GetSecurityDescriptorSacl() failed.
		dwLastError = GetLastError();
		goto out;
	}

	for (dwAceIndex = 0; dwAceIndex < sacl->AceCount; ++dwAceIndex) {
		PSYSTEM_MANDATORY_LABEL_ACE ace;
		if (GetAce(sacl, dwAceIndex, (void**)&ace) &&
		    ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
		{
			// Found the Mandatory Label ACE.
			// TODO: Also NO_WRITE_UP?
			// TODO: If the ACE wasn't found, skip SetKernelObjectSecurity()?
			ace->Mask |= SYSTEM_MANDATORY_LABEL_NO_READ_UP |
				     SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP;
			break;
		}
	}

	if (!SetKernelObjectSecurity(
		hToken,				// Handle
		LABEL_SECURITY_INFORMATION,	// SecurityInformation
		pSecurityDescriptor))		// SecurityDescriptor
	{
		// SetKernelObjectSecurity() failed.
		dwLastError = GetLastError();
		goto out;
	}

	// Success!
	dwLastError = 0;

out:
	LocalFree(pSecurityDescriptor);
	CloseHandle(hToken);
	return dwLastError;
}

/**
 * rom-properties Windows executable initialization.
 * This sets various security options.
 * References:
 * - https://docs.microsoft.com/en-us/previous-versions/bb430720(v=msdn.10)
 * - https://chromium.googlesource.com/chromium/src/+/441d852dbcb7b9b31328393c7e31562b1e268399/sandbox/win/src/process_mitigations.cc
 * - https://chromium.googlesource.com/chromium/src/+/refs/heads/master/sandbox/win/src/process_mitigations.cc
 * - https://github.com/chromium/chromium/blob/master/sandbox/win/src/process_mitigations.cc
 *
 * @param bHighSec If non-zero, enable high security for unprivileged processes.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_win32_secoptions_init(int bHighSec)
{
	HMODULE hKernel32;
#ifndef _WIN64
	pfnSetProcessDEPPolicy_t pfnSetProcessDEPPolicy;
#endif /* _WIN64 */
	pfnSetProcessMitigationPolicy_t pfnSetProcessMitigationPolicy;
	BOOL bRet;

#ifndef NDEBUG
	// Make sure this function isn't called more than once.
	static unsigned char call_count = 0;
	assert(call_count == 0);
	call_count++;
#endif /* NDEBUG */

	// KERNEL32 is always loaded, so we don't need to use
	// GetModuleHandleEx() here.
	hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != NULL);
	if (!hKernel32) {
		// Should never happen...
		return GetLastError();
	}

	/** BEGIN: Windows XP/2003 **/

	// Terminate the process if heap corruption is detected.
	// NOTE: Parameter 2 is usually type enum HEAP_INFORMATION_CLASS,
	// but this type isn't present in older versions of MinGW, so we're
	// using int instead.

	// HeapEnableTerminationOnCorruption == 1
	HeapSetInformation(NULL, 1, NULL, 0);

	// Enable DEP on 32-bit.
	// DEP is always enabled on 64-bit for 64-bit programs,
	// but on 32-bit, we might have to enable it manually.
#ifndef _WIN64
	// Enable DEP/NX.
	// NOTE: DEP/NX should be specified in the PE header
	// using ld's --nxcompat, but we'll set it manually here,
	// just in case the linker doesn't support it.

	// SetProcessDEPPolicy() was added starting with Windows XP SP3.
	pfnSetProcessDEPPolicy = (pfnSetProcessDEPPolicy_t)
		GetProcAddress(hKernel32, "SetProcessDEPPolicy");
	if (pfnSetProcessDEPPolicy) {
		pfnSetProcessDEPPolicy(PROCESS_DEP_ENABLE | PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
	} else {
		// SetProcessDEPPolicy() was not found.
		// On Windows XP SP2, we can use NtSetInformationProcess.
		// Reference: http://www.uninformed.org/?v=2&a=4
		// FIXME: Do SetDllDirectory() first if available?
		HMODULE hNtDll = GetModuleHandle(_T("ntdll.dll"));
		assert(hNtDll != NULL);
		if (hNtDll) {
			pfnNTSetInformationProcess_t pfnNtSetInformationProcess =
				(pfnNTSetInformationProcess_t)GetProcAddress(hNtDll, "NtSetInformationProcess");
			if (pfnNtSetInformationProcess) {
				ULONG dep = MEM_EXECUTE_OPTION_DISABLE |
				            MEM_EXECUTE_OPTION_PERMANENT;
				pfnNtSetInformationProcess(GetCurrentProcess(),
					ProcessExecuteFlags, &dep, sizeof(dep));
			}
		}
	}
#endif /* !_WIN64 */

	if (!IsWindowsVistaOrGreater()) {
		// We're done here.
		return 0;
	}
	/** END: Windows XP/2003 **/

	/** BEGIN: Windows Vista/7 **/

	// Harden the process's integrity level policy.
	HardenProcessIntegrityLevelPolicy();

	if (!IsWindows8OrGreater()) {
		// We're done here.
		return 0;
	}
	/** END: Windows Vista/7 **/

	/** BEGIN: Windows 8/8.1/10 **/
	// NOTE: Not separating out 8 vs. 8.1 vs. 10.

	// Check for SetProcessMitigationPolicy().
	// If available, it supercedes many of these.
	pfnSetProcessMitigationPolicy = (pfnSetProcessMitigationPolicy_t)GetProcAddress(hKernel32, "SetProcessMitigationPolicy");
	if (!pfnSetProcessMitigationPolicy) {
		// SetProcessMitigationPolicy not found...
		return -ENOENT;
	}

	// Set ASLR policy.
	{
		PROCESS_MITIGATION_ASLR_POLICY aslr = { 0 };
		aslr.EnableBottomUpRandomization = TRUE;
		aslr.EnableForceRelocateImages = TRUE;
		aslr.EnableHighEntropy = TRUE;
		aslr.DisallowStrippedImages = TRUE;
		pfnSetProcessMitigationPolicy(ProcessASLRPolicy, &aslr, sizeof(aslr));
	}

	// Set dynamic code policy.
	{
		PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic_code = { 0 };
		dynamic_code.ProhibitDynamicCode = TRUE;
#if 0
		// Added in Windows 10.0.14393 (v1607)
		// TODO: Figure out how to detect the SDK build version.
		dynamic_code.AllowThreadOptOut = FALSE;		// Win10
		dynamic_code.AllowRemoteDowngrade = FALSE;	// Win10
#endif
		pfnSetProcessMitigationPolicy(ProcessDynamicCodePolicy,
			&dynamic_code, sizeof(dynamic_code));
	}

	// Set strict handle check policy.
	{
		PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY strict_handle_check = { 0 };
		strict_handle_check.RaiseExceptionOnInvalidHandleReference = TRUE;
		strict_handle_check.HandleExceptionsPermanentlyEnabled = TRUE;
		pfnSetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy,
			&strict_handle_check, sizeof(strict_handle_check));
	}

	// Set extension point disable policy.
	// Extension point DLLs are some weird MFC-specific thing.
	// https://docs.microsoft.com/en-us/cpp/build/extension-dlls
	{
		PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_point_disable = { 0 };
		extension_point_disable.DisableExtensionPoints = TRUE;
		pfnSetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy,
			&extension_point_disable, sizeof(extension_point_disable));
	}

	// Set image load policy.
	// FIXME: Breaks if running from a network share. (unable to load libgnuintl-8.dll)
#if 0
	{
		PROCESS_MITIGATION_IMAGE_LOAD_POLICY image_load = { 0 };
		image_load.NoRemoteImages = FALSE;	// TODO
		image_load.NoLowMandatoryLabelImages = TRUE;
		image_load.PreferSystem32Images = TRUE;
		pfnSetProcessMitigationPolicy(ProcessImageLoadPolicy,
			&image_load, sizeof(image_load));
	}
#endif /* 0 */

#if defined(_MSC_VER) && _MSC_VER >= 1900 && defined(_CONTROL_FLOW_GUARD)
	// Set control flow guard policy.
	// Requires MSVC 2015+ and /guard:cf.
	{
		PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY control_flow_guard = { 0 };
		control_flow_guard.EnableControlFlowGuard = TRUE;

		// TODO: Enable export suppression? May not be available on
		// certain Windows versions, so if we enable it, fall back
		// to not-enabled if it didn't work.
		control_flow_guard.EnableExportSuppression = FALSE;
		control_flow_guard.StrictMode = FALSE;

		pfnSetProcessMitigationPolicy(ProcessControlFlowGuardPolicy,
			&control_flow_guard, sizeof(control_flow_guard));
	}
#endif /* defined(_MSC_VER) && _MSC_VER >= 1900 && defined(_CONTROL_FLOW_GUARD) */

	// Disable loading non-system fonts.
	{
		PROCESS_MITIGATION_FONT_DISABLE_POLICY font_disable = { 0 };
		font_disable.DisableNonSystemFonts = TRUE;
		font_disable.AuditNonSystemFontLoading = FALSE;
		pfnSetProcessMitigationPolicy(ProcessFontDisablePolicy,
			&font_disable, sizeof(font_disable));
	}

	if (bHighSec) {
		// High-security options that are useful for
		// non-GUI applications, e.g. rp-download.

		// Disable direct Win32k system call access.
		// This prevents direct access to NTUser/GDI system calls.
		// This is NOT usable in GUI applications.
		{
			PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY system_call_disable = { 0 };
			system_call_disable.DisallowWin32kSystemCalls = TRUE;
			//system_call_disable.AuditDisallowWin32kSystemCalls = FALSE;
			bRet = pfnSetProcessMitigationPolicy(ProcessSystemCallDisablePolicy,
				&system_call_disable, sizeof(system_call_disable));
#ifndef NDEBUG
			if (bRet) {
				OutputDebugStringA("Win32k system calls disabled!\n");
			} else {
				char buf[16];
				DWORD dwLastError = GetLastError();
				_itoa_s(dwLastError, buf, sizeof(buf), 10);
				OutputDebugStringA("Win32k system calls NOT disabled: error ");
				OutputDebugStringA(buf);
				OutputDebugStringA(" \n");
			}
#endif /* NDEBUG */
		}
	}
	/** END: Windows 8/8.1/10 **/

	return 0;
}
