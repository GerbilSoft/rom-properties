/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * secoptions_win8.h: Security options for executables. (Win8)             *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_WIN8_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_WIN8_H__

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__
#error secoptions_win8.h should not be included directly - include secoptions.h instead
#endif

/* NOTE: MinGW-w64 v5.0.3 has ProcessDynamicCodePolicy
 * defined as ProcessReserved1MitigationPolicy. */
#ifdef __GNUC__
#define ProcessDynamicCodePolicy ProcessReserved1MitigationPolicy
#endif

/**
 * NOTE: The Windows 8, 8.1, and 10 SDKs don't guard these
 * against _WIN32_WINNT < 0x0602, so we should only include
 * the redefinitions if using an older SDK.
 *
 * The actual SetProcessMitigationPolicy() function, on the
 * other hand, *is* guarded by _WIN32_WINNT >= 0x0602.
 */
#ifndef _WIN32_WINNT_WIN8
typedef enum _PROCESS_MITIGATION_POLICY {
	// Windows 8.0 only.
	// Windows 8.1 and 10 are #defined in their
	// appropriate sections.
	ProcessDEPPolicy,
	ProcessASLRPolicy,
	ProcessDynamicCodePolicy,
	ProcessStrictHandleCheckPolicy,
	ProcessSystemCallDisablePolicy,
	ProcessMitigationOptionsMask,
	ProcessExtensionPointDisablePolicy,
} PROCESS_MITIGATION_POLICY, *PPROCESS_MITIGATION_POLICY;

typedef struct _PROCESS_MITIGATION_ASLR_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD EnableBottomUpRandomization : 1;
			DWORD EnableForceRelocateImages : 1;
			DWORD EnableHighEntropy : 1;
			DWORD DisallowStrippedImages : 1;
			DWORD ReservedFlags : 28;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_ASLR_POLICY, *PPROCESS_MITIGATION_ASLR_POLICY;

typedef struct _PROCESS_MITIGATION_DEP_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD Enable : 1;
			DWORD DisableAtlThunkEmulation : 1;
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
	BOOLEAN Permanent;
} PROCESS_MITIGATION_DEP_POLICY, *PPROCESS_MITIGATION_DEP_POLICY;

typedef struct _PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD RaiseExceptionOnInvalidHandleReference : 1;
			DWORD HandleExceptionsPermanentlyEnabled : 1;
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY;

typedef struct _PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD DisallowWin32kSystemCalls : 1;
			DWORD ReservedFlags : 31;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY, *PPROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD DisableExtensionPoints : 1;
			DWORD ReservedFlags : 31;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY, *PPROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY;
#endif /* !_WIN32_WINNT_WIN8 */

/* NOTE: Not defined on MinGW-w64 v5.0.3. */
#if !defined(_WIN32_WINNT_WIN8) || defined(__GNUC__)
typedef struct _PROCESS_MITIGATION_DYNAMIC_CODE_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD ProhibitDynamicCode : 1;	// Win81
			DWORD AllowThreadOptOut : 1;	// Win10
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_DYNAMIC_CODE_POLICY, *PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY;
#endif /* !_WIN32_WINNT_WIN8 || defined(__GNUC__) */

/* NOTE: Not defined on MinGW-w64 v5.0.3. */
#if !defined(_WIN32_WINNT_WINBLUE) || defined(__GNUC__)
// Windows 8.1
#define ProcessReserved1Policy	((PROCESS_MITIGATION_POLICY)(ProcessExtensionPointDisablePolicy+1))
#define ProcessSignaturePolicy	((PROCESS_MITIGATION_POLICY)(ProcessExtensionPointDisablePolicy+2))

typedef struct _PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD MicrosoftSignedOnly : 1;
			DWORD StoreSignedOnly : 1;
			DWORD MitigationOptIn : 1;
			DWORD ReservedFlags : 29;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY, *PPROCESS_MITIGATION_BINARY_SIGNATURE_POLICY;
#endif /* !_WIN32_WINNT_WINBLUE || defined(__GNUC__) */

#ifndef _WIN32_WINNT_WIN10
// Windows 10
#define ProcessControlFlowGuardPolicy	ProcessReserved1Policy
#define ProcessFontDisablePolicy	((PROCESS_MITIGATION_POLICY)(ProcessSignaturePolicy+1))
#define ProcessImageLoadPolicy		((PROCESS_MITIGATION_POLICY)(ProcessSignaturePolicy+2))

typedef struct _PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD EnableControlFlowGuard : 1;
			DWORD ReservedFlags : 31;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY, *PPROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY;

typedef struct _PROCESS_MITIGATION_FONT_DISABLE_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD DisableNonSystemFonts     : 1;
			DWORD AuditNonSystemFontLoading : 1;
			DWORD ReservedFlags             : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_FONT_DISABLE_POLICY, *PPROCESS_MITIGATION_FONT_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_IMAGE_LOAD_POLICY {
	union {
		DWORD Flags;
		struct {
			DWORD NoRemoteImages : 1;
			DWORD NoLowMandatoryLabelImages : 1;
			DWORD PreferSystem32Images : 1;
			DWORD ReservedFlags : 29;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_IMAGE_LOAD_POLICY, *PPROCESS_MITIGATION_IMAGE_LOAD_POLICY;
#endif /* !_WIN32_WINNT_WIN10 */

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_WIN8_H__ */
