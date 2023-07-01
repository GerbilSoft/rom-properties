/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * NetworkStatus.c: Get network status.                                    *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NetworkStatus.h"

// from nldef.h
typedef enum _NL_NETWORK_CONNECTIVITY_LEVEL_HINT {
	NetworkConnectivityLevelHintUnknown = 0,
	NetworkConnectivityLevelHintNone,
	NetworkConnectivityLevelHintLocalAccess,
	NetworkConnectivityLevelHintInternetAccess,
	NetworkConnectivityLevelHintConstrainedInternetAccess,
	NetworkConnectivityLevelHintHidden
} NL_NETWORK_CONNECTIVITY_LEVEL_HINT;

typedef enum _NL_NETWORK_CONNECTIVITY_COST_HINT {
	NetworkConnectivityCostHintUnknown = 0,
	NetworkConnectivityCostHintUnrestricted,
	NetworkConnectivityCostHintFixed,
	NetworkConnectivityCostHintVariable
} NL_NETWORK_CONNECTIVITY_COST_HINT;

typedef struct _NL_NETWORK_CONNECTIVITY_HINT {
	NL_NETWORK_CONNECTIVITY_LEVEL_HINT ConnectivityLevel;
	NL_NETWORK_CONNECTIVITY_COST_HINT  ConnectivityCost;
	BOOLEAN                            ApproachingDataLimit;
	BOOLEAN                            OverDataLimit;
	BOOLEAN                            Roaming;
} NL_NETWORK_CONNECTIVITY_HINT;

// FIXME: Can't use NTSTATUS here, so use LONG instead.
#define NTSTATUS LONG
typedef NTSTATUS (WINAPI *PFNGETNETWORKCONNECTIVITYHINT)(_Out_ NL_NETWORK_CONNECTIVITY_HINT *ConnectivityHint);

/**
 * Can we identify if this system has a metered network connection?
 * @return True if we can; false if we can't.
 */
bool rp_win32_can_identify_if_metered(void)
{
	// GetNetworkConnectivityHint() was added in Windows 10 v2004.
	// Reference: https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhint
	// TODO: Cache this?
	HMODULE hIPhlpapiDll = LoadLibraryEx(_T("IPHLPAPI.DLL"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hIPhlpapiDll) {
		return false;
	}

	PFNGETNETWORKCONNECTIVITYHINT pfnGetNetworkConnectivityHint =
		(PFNGETNETWORKCONNECTIVITYHINT)GetProcAddress(hIPhlpapiDll, "GetNetworkConnectivityHint");
	FreeLibrary(hIPhlpapiDll);

	return (pfnGetNetworkConnectivityHint != NULL);
}

/**
 * Is this system using a metered network connection?
 * NOTE: If we can't identify it, this will always return false (unmetered).
 * @return True if metered; false if unmetered.
 */
bool rp_win32_is_metered(void)
{
	// Default to unmetered if we can't determine the setting.
	bool bRet = false;

	// GetNetworkConnectivityHint() was added in Windows 10 v2004.
	// Reference: https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhint
	// TODO: Cache this?
	HMODULE hIPhlpapiDll = LoadLibraryEx(_T("IPHLPAPI.DLL"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hIPhlpapiDll) {
		return bRet;
	}

	PFNGETNETWORKCONNECTIVITYHINT pfnGetNetworkConnectivityHint =
		(PFNGETNETWORKCONNECTIVITYHINT)GetProcAddress(hIPhlpapiDll, "GetNetworkConnectivityHint");
	if (!pfnGetNetworkConnectivityHint) {
		FreeLibrary(hIPhlpapiDll);
		return bRet;
	}

	NL_NETWORK_CONNECTIVITY_HINT connectivityHint;
	if (pfnGetNetworkConnectivityHint(&connectivityHint) == NO_ERROR) {
		// Obtained the network connectivity hint.
		switch (connectivityHint.ConnectivityCost) {
			default:
			case NetworkConnectivityCostHintUnknown:
			case NetworkConnectivityCostHintUnrestricted:
				// Unmetered connection, or unknown
				bRet = false;
				break;

			case NetworkConnectivityCostHintFixed:
			case NetworkConnectivityCostHintVariable:
				// Metered connection
				bRet = true;
				break;
		}
	}

	FreeLibrary(hIPhlpapiDll);
	return bRet;
}
