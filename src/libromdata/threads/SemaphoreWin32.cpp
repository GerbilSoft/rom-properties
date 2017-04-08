/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SemaphoreWin32.cpp: Win32 semaphore implementation.                     *
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

#include "Semaphore.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRomData {

/**
 * Create a semaphore.
 * @param count Number of times the semaphore can be obtained before blocking.
 */
Semaphore::Semaphore(int count)
{
	m_sem = CreateSemaphore(nullptr, count, count, nullptr);
	assert(m_sem != nullptr);
	if (!m_sem) {
		// FIXME: Do something if an error occurred here...
	}
}

/**
 * Delete the semaphore.
 * WARNING: Semaphore MUST be fully released!
 */
Semaphore::~Semaphore()
{
	if (m_sem) {
		CloseHandle(m_sem);
		m_sem = nullptr;
	}
}

/**
 * Obtain the semaphore.
 * If the semaphore is at zero, this function will block
 * until another thread releases the semaphore.
 * @return 0 on success; non-zero on error.
 */
int Semaphore::obtain(void)
{
	if (!m_sem)
		return -EBADF;

	DWORD dwWaitResult = WaitForSingleObject(m_sem, INFINITE);
	if (dwWaitResult == WAIT_OBJECT_0)
		return 0;

	// TODO: What error to return?
	return -1;
}

/**
 * Release a lock on the semaphore.
 * @return 0 on success; non-zero on error.
 */
int Semaphore::release(void)
{
	if (!m_sem)
		return -EBADF;

	BOOL bRet = ReleaseSemaphore(m_sem, 1, nullptr);
	if (bRet != 0)
		return 0;

	// TODO: What error to return?
	return -1;
}

}
