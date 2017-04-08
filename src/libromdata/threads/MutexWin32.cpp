/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MutexWin32.cpp: Win32 mutex implementation.                             *
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

#include "Mutex.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRomData {

/**
 * Create a mutex.
 */
Mutex::Mutex()
	: m_isInit(false)
{
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms686908(v=vs.85).aspx
	if (!InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x400)) {
		// FIXME: Do something if an error occurred here...
		return;
	}
	m_isInit = true;
}

/**
 * Delete the mutex.
 * WARNING: Mutex MUST be unlocked!
 */
Mutex::~Mutex()
{
	if (m_isInit) {
		// TODO: Error checking.
		DeleteCriticalSection(&m_criticalSection);
		m_isInit = false;
	}
}

/**
 * Lock the mutex.
 * If the mutex is locked, this function will block until
 * the previous locker unlocks it.
 * @return 0 on success; non-zero on error.
 */
int Mutex::lock(void)
{
	if (!m_isInit)
		return -EBADF;

	// TODO: Error handling?
	EnterCriticalSection(&m_criticalSection);
	return 0;
}

/**
 * Unlock the mutex.
 * @return 0 on success; non-zero on error.
 */
int Mutex::unlock(void)
{
	if (!m_isInit)
		return -EBADF;

	// TODO: Error handling?
	LeaveCriticalSection(&m_criticalSection);
	return 0;
}

}
