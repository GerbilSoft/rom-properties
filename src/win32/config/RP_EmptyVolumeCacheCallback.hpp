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

#ifndef __ROMPROPERTIES_WIN32_CONFIG_RP_EMPTYVOLUMECACHECALLBACK_HPP
#define __ROMPROPERTIES_WIN32_CONFIG_RP_EMPTYVOLUMECACHECALLBACK_HPP

/**
 * IEmptyVolumeCacheCallback implementation.
 * Used for thumbnail cache cleaning on Windows Vista and later.
 *
 * NOTE: This class is NOT registered with the system.
 * Therefore, we aren't defining a CLSID.
 */

#include "librpbase/common.h"
#include "RP_ComBase.hpp"

class RP_EmptyVolumeCacheCallback : public RP_ComBase<IEmptyVolumeCacheCallBack>
{
	public:
		RP_EmptyVolumeCacheCallback(HWND hProgressBar);

	private:
		typedef RP_ComBase<RP_EmptyVolumeCacheCallback> super;
		RP_DISABLE_COPY(RP_EmptyVolumeCacheCallback)

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override final;

	public:
		// IEmptyVolumeCacheCallBack
		IFACEMETHODIMP ScanProgress(DWORDLONG dwlSpaceUsed, DWORD dwFlags, LPCWSTR pcwszStatus) override final;
		IFACEMETHODIMP PurgeProgress(DWORDLONG dwlSpaceFreed, DWORDLONG dwlSpaceToFree, DWORD dwFlags, LPCWSTR pcwszStatus) override final;

	private:
		HWND m_hProgressBar;		// Progress bar to update.
	public:
		unsigned int m_baseProgress;	// Base progress value.
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_RP_EMPTYVOLUMECACHECALLBACK_HPP */
