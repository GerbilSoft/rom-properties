/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * secoptions.h: Security options for executables.                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__

#include "RpWin32_sdk.h"
#include "sdkddkver.h"

// C includes.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEP policy. (Vista SP1; later backported to XP SP3)
typedef BOOL (WINAPI *PFNSETPROCESSDEPPOLICY)(_In_ DWORD dwFlags);
#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#endif
#ifndef PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION 0x2
#endif

// SetDllDirectory() (Win2003; later backported to XP SP1)
typedef BOOL (WINAPI *PFNSETDLLDIRECTORYW)(_In_opt_ LPCWSTR lpPathName);

// SetDefaultDllDirectories() (Win8; later backported to Vista and Win7)
typedef BOOL (WINAPI *PFNSETDEFAULTDLLDIRECTORIES)(_In_ DWORD DirectoryFlags);
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif

// HeapSetInformation() (WinXP)
typedef BOOL (WINAPI *PFNHEAPSETINFORMATION)
	(_In_opt_ HANDLE HeapHandle,
	 _In_ int HeapInformationClass,
	 _In_ PVOID HeapInformation,
	 _In_ SIZE_T HeapInformationLength);

// SetProcessMitigationPolicy (Win8)
// Reference: https://git.videolan.org/?p=vlc/vlc-2.2.git;a=commitdiff;h=054cf24557164f79045d773efe7da87c4fe357de;hp=52e4b740ad47574bdff7b80aba4949311e1b88f1
#include "secoptions_win8.h"
typedef BOOL (WINAPI *PFNSETPROCESSMITIGATIONPOLICY)(_In_ PROCESS_MITIGATION_POLICY MitigationPolicy, _In_ PVOID lpBuffer, _In_ SIZE_T dwLength);

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __inline
#else /* !_MSC_VER */
#define INLINE inline
#endif /* _MSC_VER */
#endif /* !INLINE */

/**
 * libromdata Windows executable initialization.
 * This sets various security options.
 * Reference: http://msdn.microsoft.com/en-us/library/bb430720.aspx
 *
 * @param bHighSec If non-zero, enable high security for unprivileged processes.
 * @return 0 on success; non-zero on error.
 */
static INLINE int secoptions_init(void)
{
	BOOL bHighSec = FALSE;	// TODO: Move back to a parameter.
	HMODULE hKernel32;
	PFNSETPROCESSMITIGATIONPOLICY pfnSetProcessMitigationPolicy;
	PFNSETDLLDIRECTORYW pfnSetDllDirectoryW;
	PFNSETDEFAULTDLLDIRECTORIES pfnSetDefaultDllDirectories;
	PFNHEAPSETINFORMATION pfnHeapSetInformation;

#ifndef NDEBUG
	// Make sure this function isn't called more than once.
	static unsigned char call_count = 0;
	assert(call_count == 0);
	call_count++;
#endif /* NDEBUG */

	// KERNEL32 is always loaded, so we don't need to use
	// GetModuleHandleEx() here.
	hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != nullptr);
	if (!hKernel32) {
		// Should never happen...
		return GetLastError();
	}

	// Check for SetProcessMitigationPolicy().
	// If available, it supercedes many of these.
	pfnSetProcessMitigationPolicy = (PFNSETPROCESSMITIGATIONPOLICY)GetProcAddress(hKernel32, "SetProcessMitigationPolicy");
	if (pfnSetProcessMitigationPolicy) {
		union {
			// TODO: Some of these need further investigation.
			DWORD flags;
			PROCESS_MITIGATION_DEP_POLICY dep;
			PROCESS_MITIGATION_ASLR_POLICY aslr;
			PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic_code;
			PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY strict_handle_check;
			PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY system_call_disable;
			PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_point_disable;
			PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY control_flow_guard;	// MSVC 2015+: /guard:cf
			//PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY binary_signature;
			PROCESS_MITIGATION_FONT_DISABLE_POLICY font_disable;
			PROCESS_MITIGATION_IMAGE_LOAD_POLICY image_load;
		} policy;
		// Most of these are 4 bytes, except for
		// PROCESS_MITIGATION_DEP_POLICY, which is 8.
		static_assert(sizeof(policy) == 8, "sizeof(policy) != 8");

		// Set DEP policy.
		policy.flags = 0;
		policy.dep.Enable = 1;
		policy.dep.DisableAtlThunkEmulation = 1;
		policy.dep.Permanent = TRUE;
		pfnSetProcessMitigationPolicy(ProcessDEPPolicy,
			&policy.dep, sizeof(policy.dep));

		// Set ASLR policy.
		policy.flags = 0;
		policy.aslr.EnableBottomUpRandomization = 1;
		policy.aslr.EnableForceRelocateImages = 1;
		policy.aslr.EnableHighEntropy = 1;
		policy.aslr.DisallowStrippedImages = 1;
		pfnSetProcessMitigationPolicy(ProcessASLRPolicy,
			&policy.aslr, sizeof(policy.aslr));

		// Set dynamic code policy.
		policy.flags = 0;
		policy.dynamic_code.ProhibitDynamicCode = 1;
#if 0
		// Added in Windows 10.0.14393 (v1607)
		// TODO: Figure out how to detect the SDK build version.
		policy.dynamic_code.AllowThreadOptOut = 0;	// Win10
		policy.dynamic_code.AllowRemoteDowngrade = 0;	// Win10
#endif
		pfnSetProcessMitigationPolicy(ProcessDynamicCodePolicy,
			&policy.dynamic_code, sizeof(policy.dynamic_code));

		// Set strict handle check policy.
		policy.flags = 0;
		policy.strict_handle_check.RaiseExceptionOnInvalidHandleReference = 1;
		policy.strict_handle_check.HandleExceptionsPermanentlyEnabled = 1;
		pfnSetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy,
			&policy.strict_handle_check, sizeof(policy.strict_handle_check));

		// Set extension point disable policy.
		// Extension point DLLs are some weird MFC-specific thing.
		// https://msdn.microsoft.com/en-us/library/h5f7ck28.aspx
		policy.flags = 0;
		policy.extension_point_disable.DisableExtensionPoints = 1;
		pfnSetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy,
			&policy.extension_point_disable, sizeof(policy.extension_point_disable));

		// Set image load policy.
		policy.flags = 0;
		policy.image_load.NoRemoteImages = 0;	// TODO
		policy.image_load.NoLowMandatoryLabelImages = 1;
		policy.image_load.PreferSystem32Images = 1;
		pfnSetProcessMitigationPolicy(ProcessImageLoadPolicy,
			&policy.image_load, sizeof(policy.image_load));

#if defined(_MSC_VER) && _MSC_VER >= 1900
		// Set control flow guard policy.
		// TODO: Enable export suppression? May not be available on
		// certain Windows versions, so if we enable it, fall back
		// to not-enabled if it didn't work.
		policy.flags = 0;
		policy.control_flow_guard.EnableControlFlowGuard = 1;
		pfnSetProcessMitigationPolicy(ProcessControlFlowGuardPolicy,
			&policy.control_flow_guard, sizeof(policy.control_flow_guard));
#endif /* defined(_MSC_VER) && _MSC_VER >= 1900 */

		if (bHighSec) {
			// High-security options that are useful for
			// non-GUI applications, e.g. rp-download.

			// Disable direct Win32k system call access.
			// This prevents direct access to NTUser/GDI system calls.
			// This is NOT usable in GUI applications.
			policy.flags = 0;
			policy.system_call_disable.DisallowWin32kSystemCalls = 1;
			pfnSetProcessMitigationPolicy(ProcessSystemCallDisablePolicy,
				&policy.system_call_disable, sizeof(policy.system_call_disable));

			// Disable loading non-system fonts.
			policy.flags = 0;
			policy.font_disable.DisableNonSystemFonts = 1;
			policy.font_disable.AuditNonSystemFontLoading = 0;
			pfnSetProcessMitigationPolicy(ProcessFontDisablePolicy,
				&policy.font_disable, sizeof(policy.font_disable));
		}
	} else {
		// Use the old functions if they're available.
		PFNSETPROCESSDEPPOLICY pfnSetProcessDEPPolicy;

		// Enable DEP/NX.
		// NOTE: DEP/NX should be specified in the PE header
		// using ld's --nxcompat, but we'll set it manually here,
		// just in case the linker doesn't support it.
		pfnSetProcessDEPPolicy = (PFNSETPROCESSDEPPOLICY)GetProcAddress(hKernel32, "SetProcessDEPPolicy");
		if (pfnSetProcessDEPPolicy) {
			pfnSetProcessDEPPolicy(PROCESS_DEP_ENABLE | PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
		}
	}

	// Remove the current directory from the DLL search path.
	pfnSetDllDirectoryW = (PFNSETDLLDIRECTORYW)GetProcAddress(hKernel32, "SetDllDirectoryW");
	if (pfnSetDllDirectoryW) {
		//pfnSetDllDirectoryW(L"");
	}

	// Only search the system directory for DLLs.
	// The Delay-Load helper will handle bundled DLLs at runtime.
	// NOTE: gdiplus.dll is not a "Known DLL", and since it isn't
	// delay-loaded, it may be loaded from the application directory...
	pfnSetDefaultDllDirectories = (PFNSETDEFAULTDLLDIRECTORIES)GetProcAddress(hKernel32, "SetDefaultDllDirectories");
	if (pfnSetDefaultDllDirectories) {
		//pfnSetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
	}

	// Terminate the process if heap corruption is detected.
	// NOTE: Parameter 2 is usually type enum HEAP_INFORMATION_CLASS,
	// but this type isn't present in older versions of MinGW, so we're
	// using int instead.
	pfnHeapSetInformation = (PFNHEAPSETINFORMATION)GetProcAddress(hKernel32, "HeapSetInformation");
	if (pfnHeapSetInformation) {
		// HeapEnableTerminationOnCorruption == 1
		pfnHeapSetInformation(NULL, 1, NULL, 0);
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__ */
