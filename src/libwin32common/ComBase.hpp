/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.hpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
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

#define RP_COMBASE_IMPL(name) \
{ \
	protected: \
		/* References of this object */ \
		volatile long m_lRefCount; \
	\
	public: \
		name() : m_lRefCount(1) \
		{ \
			incRpGlobalRefCount(); \
		} \
		virtual ~name() \
		{ \
			assert(m_lRefCount == 0); \
		} \
	\
	public: \
		/** IUnknown **/ \
		IFACEMETHODIMP_(ULONG) AddRef(void) final \
		{ \
			incRpGlobalRefCount(); \
			return static_cast<ULONG>(_InterlockedIncrement(&m_lRefCount)); \
		} \
		\
		IFACEMETHODIMP_(ULONG) Release(void) final \
		{ \
			assert(m_lRefCount > 0); \
			long lRefCount = _InterlockedDecrement(&m_lRefCount); \
			if (lRefCount == 0) { \
				/* No more references */ \
				delete this; \
			} \
			decRpGlobalRefCount(); \
			return static_cast<ULONG>(lRefCount); \
		} \
}

template<class I>
class NOVTABLE ComBase : public I
	RP_COMBASE_IMPL(ComBase);

template<class I1, class I2>
class NOVTABLE ComBase2 : public I1, public I2
	RP_COMBASE_IMPL(ComBase2);

template<class I1, class I2, class I3>
class NOVTABLE ComBase3 : public I1, public I2, public I3
	RP_COMBASE_IMPL(ComBase3);

} //namespace LibWin32Common
