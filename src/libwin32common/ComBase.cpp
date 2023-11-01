/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.cpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "ComBase.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <stdio.h>

// IUnknown
#include <unknwn.h>

namespace LibWin32Common {

// References of all objects.
static volatile ULONG RP_ulTotalRefCount = 0;

void incRpGlobalRefCount(void)
{
	InterlockedIncrement(&RP_ulTotalRefCount);
}

void decRpGlobalRefCount(void)
{
	InterlockedDecrement(&RP_ulTotalRefCount);
}

/**
 * Is an RP_ComBase object referenced?
 * @return True if RP_ulTotalRefCount > 0; false if not.
 */
bool ComBase_isReferenced(void)
{
	return (RP_ulTotalRefCount > 0);
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

	for (const QITAB *p = pqit; p->piid != nullptr; ++p) {
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

} //namespace LibWin32Common
