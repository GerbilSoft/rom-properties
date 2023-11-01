/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.hpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
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

#include <assert.h>

// QISearch()
#include "sdk/QITab.h"

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

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
		/* References of this object. */ \
		volatile ULONG m_ulRefCount; \
	\
	public: \
		name() : m_ulRefCount(1) \
		{ \
			incRpGlobalRefCount(); \
		} \
		virtual ~name() \
		{ \
			assert(m_ulRefCount == 0); \
		} \
	\
	public: \
		/** IUnknown **/ \
		IFACEMETHODIMP_(ULONG) AddRef(void) final \
		{ \
			incRpGlobalRefCount(); \
			InterlockedIncrement(&m_ulRefCount); \
			return m_ulRefCount; \
		} \
		\
		IFACEMETHODIMP_(ULONG) Release(void) final \
		{ \
			assert(m_ulRefCount > 0); \
			ULONG ulRefCount = InterlockedDecrement(&m_ulRefCount); \
			if (ulRefCount == 0) { \
				/* No more references. */ \
				delete this; \
			} \
			decRpGlobalRefCount(); \
			return ulRefCount; \
		} \
}

template<class I>
class ComBase : public I
	RP_COMBASE_IMPL(ComBase);

template<class I1, class I2>
class ComBase2 : public I1, public I2
	RP_COMBASE_IMPL(ComBase2);

template<class I1, class I2, class I3>
class ComBase3 : public I1, public I2, public I3
	RP_COMBASE_IMPL(ComBase3);

} //namespace LibWin32Common
