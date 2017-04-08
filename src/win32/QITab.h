/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * QITab.h: QITAB header.                                                  *
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

#ifndef __ROMPROPERTIES_WIN32_QITAB_H__
#define __ROMPROPERTIES_WIN32_QITAB_H__

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OFFSETOFCLASS
// Get the class vtable offset. Used for QITAB.
// NOTE: QITAB::dwOffset is int, not DWORD.
#define OFFSETOFCLASS(base, derived) \
    (int)((DWORD)(DWORD_PTR)((base*)((derived*)8))-8)
#endif

#ifndef QITABENT
// QITAB is not defined on MinGW-w64 4.0.6.
typedef struct _QITAB {
	const IID *piid;
	int dwOffset;
} QITAB, *LPQITAB;
typedef const QITAB *LPCQITAB;

#ifdef __cplusplus
# define QITABENTMULTI(Cthis, Ifoo, Iimpl) \
    { &__uuidof(Ifoo), OFFSETOFCLASS(Iimpl, Cthis) }
#else
# define QITABENTMULTI(Cthis, Ifoo, Iimpl) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }
#endif /* __cplusplus */

#define QITABENTMULTI2(Cthis, Ifoo, Iimpl) \
    { (IID*) &Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }

#define QITABENT(Cthis, Ifoo) QITABENTMULTI(Cthis, Ifoo, Ifoo)
#endif /* QITABENT */

// QISearch() function pointer.
typedef HRESULT (STDAPICALLTYPE *PFNQISEARCH)(_Inout_ void* that, _In_ LPCQITAB pqit, _In_ REFIID riid, _COM_Outptr_ void **ppv);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_WIN32_QITAB_H__ */
