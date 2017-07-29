/********************************************************************************
 * ROM Properties Page shell extension. (Win32)                                 *
 * RP_EmptyVolumeCacheCallback.cpp: RP_EmptyVolumeCacheCallback implementation. *
 *                                                                              *
 * Copyright (c) 2016-2017 by David Korth.                                      *
 *                                                                              *
 * This program is free software; you can redistribute it and/or modify it      *
 * under the terms of the GNU General Public License as published by the        *
 * Free Software Foundation; either version 2 of the License, or (at your       *
 * option) any later version.                                                   *
 *                                                                              *
 * This program is distributed in the hope that it will be useful, but          *
 * WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License along      *
 * with this program; if not, write to the Free Software Foundation, Inc.,      *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.                *
 ********************************************************************************/

#include "stdafx.h"
#include "RP_EmptyVolumeCacheCallback.hpp"

RP_EmptyVolumeCacheCallback::RP_EmptyVolumeCacheCallback(HWND hProgressBar)
	: m_hProgressBar(hProgressBar)
	, m_baseProgress(0)
{ }

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_EmptyVolumeCacheCallback::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_EmptyVolumeCacheCallback, IEmptyVolumeCacheCallBack),
		{ 0 }
	};
	return LibWin32Common::pQISearch(this, rgqit, riid, ppvObj);
}

/** IEmptyVolumeCacheCallBack **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb762008(v=vs.85).aspx
// TODO: Needs testing on a system with lots of thumbnails.

IFACEMETHODIMP RP_EmptyVolumeCacheCallback::ScanProgress(DWORDLONG dwlSpaceUsed, DWORD dwFlags, LPCWSTR pcwszStatus)
{
	return S_OK;
}

IFACEMETHODIMP RP_EmptyVolumeCacheCallback::PurgeProgress(DWORDLONG dwlSpaceFreed, DWORDLONG dwlSpaceToFree, DWORD dwFlags, LPCWSTR pcwszStatus)
{
	// TODO: Verify that the UI gets updated here.
	if (!m_hProgressBar) {
		return S_OK;
	}
	// TODO: Better way to calculate a percentage?
	float fPct = (float)dwlSpaceFreed / (float)dwlSpaceToFree;
	SendMessage(m_hProgressBar, PBM_SETSTATE, m_baseProgress + (unsigned int)(fPct * 100.0f), 0);
	return S_OK;
}
