/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_windres.cpp: Windows resource wrapper for IRpFile. (Win32)       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpFile_windres.hpp"

// libwin32common
#include "libwin32common/w32err.hpp"

// C++ STL classes
using std::string;

/**
 * Open an IRpFile backed by a Win32 resource.
 * The resulting IRpFile is read-only.
 *
 * @param buf Memory buffer.
 * @param size Size of memory buffer.
 */
RpFile_windres::RpFile_windres(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType)
	: super()
	, m_hGlobal(nullptr)
{
	assert(lpName != nullptr);
	assert(lpType != nullptr);
	if (!lpName || !lpType) {
		// No resource specified.
		m_lastError = EBADF;
		return;
	}

	// Open the resource.
	HRSRC hRsrc = FindResource(hModule, lpName, lpType);
	if (!hRsrc) {
		// Resource not found.
		m_lastError = w32err_to_posix(GetLastError());
		return;
	}

	// Get the resource size.
	m_size = static_cast<off64_t>(SizeofResource(hModule, hRsrc));
	if (m_size <= 0) {
		// Unable to get the resource size.
		m_lastError = w32err_to_posix(GetLastError());
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		return;
	}

	// Load the resource.
	m_hGlobal = LoadResource(hModule, hRsrc);
	if (!m_hGlobal) {
		// Unable to load the resource.
		m_lastError = w32err_to_posix(GetLastError());
		m_hGlobal = nullptr;
		return;
	}

	// Lock the resource.
	// (Technically not needed on Win32...)
	m_buf = LockResource(m_hGlobal);
	if (!m_buf) {
		// Failed to lock the resource.
		m_lastError = w32err_to_posix(GetLastError());
		FreeResource(m_hGlobal);
		m_hGlobal = nullptr;
		return;
	}

	// Resource is loaded.
}

RpFile_windres::~RpFile_windres()
{
	if (m_hGlobal) {
		UnlockResource(m_buf);
		FreeResource(m_hGlobal);
	}
}

/**
 * Close the file.
 */
void RpFile_windres::close(void)
{
	if (m_hGlobal) {
		UnlockResource(m_buf);
		FreeResource(m_hGlobal);
		m_hGlobal = nullptr;
		super::close();
	}
}
