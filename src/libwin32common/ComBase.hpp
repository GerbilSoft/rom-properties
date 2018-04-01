/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.hpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_COMBASE_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_COMBASE_HPP__

/**
 * COM base class. Handles reference counting and IUnknown.
 * References:
 * - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
 * - http://www.codeproject.com/Articles/338268/COM-in-C
 * - http://stackoverflow.com/questions/17310733/how-do-i-re-use-an-interface-implementation-in-many-classes
 */

// QISearch()
#include "sdk/QITab.h"

namespace LibWin32Common {

// QISearch() function pointer.
extern PFNQISEARCH pQISearch;

// References of all objects.
extern volatile ULONG RP_ulTotalRefCount;

// Manipulate the global COM reference count.
void incRpGlobalRefCount(void);
void decRpGlobalRefCount(void);

/**
 * Is an RP_ComBase object referenced?
 * @return True if RP_ulTotalRefCount > 0; false if not.
 */
static inline bool ComBase_isReferenced(void)
{
	return (RP_ulTotalRefCount > 0);
}

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
		virtual ~name() { } \
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

}

#endif /* __ROMPROPERTIES_WIN32_RP_COMBASE_HPP__ */
