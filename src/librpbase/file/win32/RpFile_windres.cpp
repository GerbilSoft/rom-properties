/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_windres.cpp: Windows resource wrapper for IRpFile. (Win32)       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpFile_windres.hpp"

// libwin32common
#include "libwin32common/w32err.h"

// C++ STL classes.
using std::string;

namespace LibRpBase {

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
	, m_lpData(nullptr)
	, m_dwSize(0)
	, m_dwPos(0)
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
	m_dwSize = SizeofResource(hModule, hRsrc);
	if (m_dwSize == 0) {
		// Unable to get the resource size.
		m_lastError = w32err_to_posix(GetLastError());
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
	m_lpData = LockResource(m_hGlobal);
	if (!m_lpData) {
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
	if (m_lpData) {
		UnlockResource(m_lpData);
		FreeResource(m_hGlobal);
	}
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile_windres::isOpen(void) const
{
	return (m_lpData != nullptr);
}

/**
 * Close the file.
 */
void RpFile_windres::close(void)
{
	if (m_lpData) {
		UnlockResource(m_lpData);
		FreeResource(m_hGlobal);
		m_lpData = nullptr;
		m_hGlobal = nullptr;
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile_windres::read(void *ptr, size_t size)
{
	if (!m_lpData) {
		m_lastError = EBADF;
		return 0;
	}

	if (unlikely(size == 0)) {
		// Not reading anything...
		return 0;
	}

	// Check if size is in bounds.
	// NOTE: Need to use a signed comparison here.
	if (static_cast<int64_t>(m_dwPos) > (static_cast<int64_t>(m_dwSize) - static_cast<int64_t>(size))) {
		// Not enough data.
		// Copy whatever's left in the buffer.
		size = m_dwSize - m_dwPos;
	}

	// Copy the data.
	const uint8_t *const buf = static_cast<const uint8_t*>(m_lpData);
	memcpy(ptr, &buf[m_dwPos], size);
	m_dwPos += static_cast<DWORD>(size);
	return size;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for RpFile_windres; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile_windres::write(const void *ptr, size_t size)
{
	// Not a valid operation for RpFile_windres.
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	m_lastError = EBADF;
	return 0;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile_windres::seek(int64_t pos)
{
	if (!m_lpData) {
		m_lastError = EBADF;
		return -1;
	}

	// NOTE: m_pos is size_t, since it's referring to
	// a position within a memory buffer.
	if (pos <= 0) {
		m_dwPos = 0;
	} else if (static_cast<DWORD>(pos) >= m_dwSize) {
		m_dwPos = m_dwSize;
	} else {
		m_dwPos = static_cast<DWORD>(pos);
	}

	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpFile_windres::tell(void)
{
	if (!m_lpData) {
		m_lastError = EBADF;
		return 0;
	}

	return static_cast<int64_t>(m_dwPos);
}

/**
 * Truncate the file.
 * (NOTE: Not valid for RpFile_windres; this will always return -1.)
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile_windres::truncate(int64_t size)
{
	// Not supported.
	// TODO: Writable RpFile_windres?
	RP_UNUSED(size);
	m_lastError = ENOTSUP;
	return -1;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile_windres::size(void)
{
	if (!m_lpData) {
		m_lastError = EBADF;
		return -1;
	}

	return static_cast<int64_t>(m_dwSize);
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
string RpFile_windres::filename(void) const
{
	// TODO: Implement this?
	return string();
}

}
