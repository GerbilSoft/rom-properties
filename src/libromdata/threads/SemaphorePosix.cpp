/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SemaphorePosix.cpp: POSIX semaphore implementation.                     *
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
	: m_isInit(false)
{
	int ret = sem_init(&m_sem, 0, count);
	assert(ret == 0);
	if (ret == 0) {
		m_isInit = true;
	} else {
		// FIXME: Do something if an error occurred here...
	}
}

/**
 * Delete the semaphore.
 * WARNING: Semaphore MUST be fully released!
 */
Semaphore::~Semaphore()
{
	if (m_isInit) {
		// TODO: Error checking.
		sem_destroy(&m_sem);
		m_isInit = false;
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
	if (!m_isInit)
		return -EBADF;

	// TODO: What error to return?
	return sem_wait(&m_sem);
}

/**
 * Release a lock on the semaphore.
 * @return 0 on success; non-zero on error.
 */
int Semaphore::release(void)
{
	if (!m_isInit)
		return -EBADF;

	// TODO: What error to return?
	return sem_post(&m_sem);
}

}
