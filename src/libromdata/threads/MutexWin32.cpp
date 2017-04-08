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
{
	m_mutex = CreateMutex(nullptr, FALSE, nullptr);
	assert(m_mutex != nullptr);
	if (!m_mutex) {
		// FIXME: Do something if an error occurred here...
	}
}

/**
 * Delete the mutex.
 * WARNING: Mutex MUST be unlocked!
 */
Mutex::~Mutex()
{
	if (m_mutex) {
		CloseHandle(m_mutex);
		m_mutex = nullptr;
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
	if (!m_mutex)
		return -EBADF;

	DWORD dwWaitResult = WaitForSingleObject(m_mutex, INFINITE);
	if (dwWaitResult == WAIT_OBJECT_0)
		return 0;

	// TODO: What error to return?
	return -1;
}

/**
 * Unlock the mutex.
 * @return 0 on success; non-zero on error.
 */
int Mutex::unlock(void)
{
	if (!m_mutex)
		return -EBADF;

	BOOL bRet = ReleaseMutex(m_mutex);
	if (bRet != 0)
		return 0;

	// TODO: What error to return?
	return -1;
}

}
