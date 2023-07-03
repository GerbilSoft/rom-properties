/********************************************************************************
 * ROM Properties Page shell extension. (Win32)                                 *
 * RP_EmptyVolumeCacheCallback.cpp: RP_EmptyVolumeCacheCallback implementation. *
 *                                                                              *
 * Copyright (c) 2016-2023 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                                    *
 ********************************************************************************/

#include "stdafx.h"
#include "RP_EmptyVolumeCacheCallback.hpp"

RP_EmptyVolumeCacheCallback::RP_EmptyVolumeCacheCallback(HWND hProgressBar)
	: m_hProgressBar(hProgressBar)
	, m_baseProgress(0)
{ }

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_EmptyVolumeCacheCallback::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_EmptyVolumeCacheCallback, IEmptyVolumeCacheCallBack),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IEmptyVolumeCacheCallBack **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/emptyvc/nn-emptyvc-iemptyvolumecachecallback
// TODO: Needs testing on a system with lots of thumbnails.

IFACEMETHODIMP RP_EmptyVolumeCacheCallback::ScanProgress(DWORDLONG dwlSpaceUsed, DWORD dwFlags, LPCWSTR pcwszStatus)
{
	RP_UNUSED(dwlSpaceUsed);
	RP_UNUSED(dwFlags);
	RP_UNUSED(pcwszStatus);
	return S_OK;
}

IFACEMETHODIMP RP_EmptyVolumeCacheCallback::PurgeProgress(DWORDLONG dwlSpaceFreed, DWORDLONG dwlSpaceToFree, DWORD dwFlags, LPCWSTR pcwszStatus)
{
	RP_UNUSED(dwFlags);
	RP_UNUSED(pcwszStatus);

	// TODO: Verify that the UI gets updated here.
	if (!m_hProgressBar) {
		return S_OK;
	}

	// TODO: Better way to calculate a percentage?
	// TODO: If dwFlags has EVCCBF_LASTNOTIFICATION, show 100% unconditionally?
	float fPct = static_cast<float>(dwlSpaceFreed) / static_cast<float>(dwlSpaceToFree);
	SendMessage(m_hProgressBar, PBM_SETSTATE, m_baseProgress + static_cast<unsigned int>(fPct * 100.0f), 0);
	return S_OK;
}
