/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.cpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "ComBase.hpp"
#include "HiDPI.h"

// C includes. (C++ namespace)
#include <cassert>

// librpthreads
#include "librpthreads/pthread_once.h"

// IUnknown
#include <unknwn.h>

namespace LibWin32Common {

// References of all objects.
volatile ULONG RP_ulTotalRefCount = 0;

/** Dynamically loaded common functions **/
static volatile pthread_once_t combase_once_control = PTHREAD_ONCE_INIT;

// IsThemeActive()
static HMODULE hUxTheme_dll = nullptr;
PFNISTHEMEACTIVE pfnIsThemeActive = nullptr;

static void initFnPtrs(void)
{
	// UXTHEME.DLL!IsThemeActive
	hUxTheme_dll = LoadLibrary(_T("uxtheme.dll"));
	assert(hUxTheme_dll != nullptr);
	if (hUxTheme_dll) {
		pfnIsThemeActive = reinterpret_cast<PFNISTHEMEACTIVE>(GetProcAddress(hUxTheme_dll, "IsThemeActive"));
		if (!pfnIsThemeActive) {
			FreeLibrary(hUxTheme_dll);
			hUxTheme_dll = nullptr;
		}
	}
}

void incRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedIncrement(&RP_ulTotalRefCount);

	// Make sure the function pointers are initialized.
	pthread_once((pthread_once_t*)&combase_once_control, initFnPtrs);
}

void decRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedDecrement(&RP_ulTotalRefCount);
	if (ulRefCount != 0)
		return;

	// Last Release(). Unload the function pointers.
	// NOTE: combase_once_control should not be PTHREAD_ONCE_INIT here.
	assert(combase_once_control != PTHREAD_ONCE_INIT);
	// This is always correct for our pthread_once() implementation.
	while (combase_once_control != 1) {
		SwitchToThread();
	}

	if (hUxTheme_dll) {
		pfnIsThemeActive = nullptr;
		FreeLibrary(hUxTheme_dll);
		hUxTheme_dll = nullptr;
	}

	// Unload modules needed for High-DPI, if necessary.
	rp_DpiUnloadModules();

	// Finished unloading function pointers.
	combase_once_control = PTHREAD_ONCE_INIT;
}

/**
 * QISearch() implementation.
 *
 * Normally implemented in shlwapi.dll, but not exported by name prior to Windows Vista.
 * (Later versions of Windows move QISearch() to kernelbase.dll and use a forwarder.)
 *
 * Based on wine's implementation:
 * https://github.com/wine-mirror/wine/blob/1bb953c6766c9cc4372ca23a7c5b7de101324218/dlls/kernelbase/main.c#L218
 *
 * @param that	[in] Base class pointer.
 * @param pqit	[in] QITAB. (Must be NULL-terminated.)
 * @param riid	[in] IID of the interface to retrieve.
 * @param ppv	[out] Output pointer.
 * @return S_OK if the requested interface was found; E_NOINTERFACE if not found.
 */
HRESULT WINAPI rp_QISearch(_Inout_ void *that, _In_ LPCQITAB pqit, _In_ REFIID riid, _COM_Outptr_ void **ppv)
{
	assert(that != nullptr);
	assert(ppv != nullptr);
	if (!that || !ppv) {
		return E_POINTER;
	}

	for (const QITAB *p = pqit; p->piid != NULL; ++p) {
		if (IsEqualIID(riid, *p->piid)) {
			// Found a matching IID.
			IUnknown *const pUnk = reinterpret_cast<IUnknown*>(
				reinterpret_cast<BYTE*>(that) + p->dwOffset);
			*ppv = pUnk;
			pUnk->AddRef();
			return S_OK;
		}
	}

	// Not found. Check if this is IUnknown.
	if (IsEqualIID(riid, IID_IUnknown)) {
		// This is IUnknown. Use the first interface.
		IUnknown *const pUnk = reinterpret_cast<IUnknown*>(
			reinterpret_cast<BYTE*>(that) + pqit->dwOffset);
		*ppv = pUnk;
		pUnk->AddRef();
		return S_OK;
	}

	// Not IUnknown. Interface is not supported.
	*ppv = nullptr;
	return E_NOINTERFACE;
}

}
