/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.hpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/**
 * COM base class. Handles reference counting and IUnknown.
 * References:
 * - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
 * - http://www.codeproject.com/Articles/338268/COM-in-C
 * - http://stackoverflow.com/questions/17310733/how-do-i-re-use-an-interface-implementation-in-many-classes
 */

// C includes (C++ namespace)
#include <cassert>

// QISearch()
#include "sdk/QITab.h"

#include "common.h"	// for NOVTABLE
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// MSVC intrinsics
#include <intrin.h>

// Common COM headers included by all COM objects.
#include <comdef.h>
#include <shlobj.h>

namespace LibWin32Common {

// Manipulate the global COM reference count.
RP_LIBROMDATA_PUBLIC void incRpGlobalRefCount(void);
RP_LIBROMDATA_PUBLIC void decRpGlobalRefCount(void);

/**
 * Is an RP_ComBase object referenced?
 * @return True if RP_ulTotalRefCount > 0; false if not.
 */
// References of all objects.
RP_LIBROMDATA_PUBLIC
bool ComBase_isReferenced(void);

// QISearch() [our own implementation]
RP_LIBROMDATA_PUBLIC
HRESULT WINAPI rp_QISearch(_Inout_ void *that, _In_ LPCQITAB pqit, _In_ REFIID riid, _COM_Outptr_ void **ppv);

template<class... I>
class NOVTABLE ComBase : public I...
{
protected:
	// References of this object
	volatile long m_lRefCount;

public:
	ComBase() : m_lRefCount(1)
	{
		incRpGlobalRefCount();
	}
	virtual ~ComBase()
	{
		assert(m_lRefCount == 0);
	}

public:
	// IUnknown
	IFACEMETHODIMP_(ULONG) AddRef(void) final
	{
		incRpGlobalRefCount();
		return static_cast<ULONG>(_InterlockedIncrement(&m_lRefCount));
	}

	IFACEMETHODIMP_(ULONG) Release(void) final
	{
		assert(m_lRefCount > 0);
		long lRefCount = _InterlockedDecrement(&m_lRefCount);
		if (lRefCount == 0) {
			// No more references
			delete this;
		}
		decRpGlobalRefCount();
		return static_cast<ULONG>(lRefCount);
	}
};

} // namespace LibWin32Common
