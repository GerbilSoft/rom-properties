/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SemaphoreMac.cpp: Mac OS X semaphore implementation.                    *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "Semaphore.hpp"
#include <mach/mach_traps.h>
#include <mach/semaphore.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// References:
// - https://developer.apple.com/library/content/documentation/Darwin/Conceptual/KernelProgramming/synchronization/synchronization.html
// - https://gist.github.com/kazupon/3843288

namespace LibRpBase {

class Semaphore
{
	public:
		/**
		 * Create a semaphore.
		 * @param count Number of times the semaphore can be obtained before blocking.
		 */
		inline explicit Semaphore(int count);

		/**
		 * Delete the semaphore.
		 * WARNING: Semaphore MUST be fully released!
		 */
		inline ~Semaphore();

	private:
		RP_DISABLE_COPY(Semaphore)

	public:
		/**
		 * Obtain the semaphore.
		 * If the semaphore is at zero, this function will block
		 * until another thread releases the semaphore.
		 * @return 0 on success; non-zero on error.
		 */
		inline int obtain(void);

		/**
		 * Release a lock on the semaphore.
		 * @return 0 on success; non-zero on error.
		 */
		inline int release(void);

	private:
		semaphore_t m_sem;
};

/**
 * Create a semaphore.
 * @param count Number of times the semaphore can be obtained before blocking.
 */
inline Semaphore::Semaphore(int count)
	: m_sem(0)
{
	kern_return_t ret = semaphore_create(mach_task_self(), &m_sem, SYNC_POLICY_FIFO, count);
	assert(ret == KERN_SUCCESS);
	assert(m_sem != 0);
	// FIXME: Do something if an error occurred here...
}

/**
 * Delete the semaphore.
 * WARNING: Semaphore MUST be fully released!
 */
inline Semaphore::~Semaphore()
{
	if (m_sem != 0) {
		semaphore_destroy(mach_task_self(), m_sem);
	}
}

/**
 * Obtain the semaphore.
 * If the semaphore is at zero, this function will block
 * until another thread releases the semaphore.
 * @return 0 on success; non-zero on error.
 */
inline int Semaphore::obtain(void)
{
	if (m_sem != 0)
		return -EBADF;

	return semaphore_wait(m_sem);
}

/**
 * Release a lock on the semaphore.
 * @return 0 on success; non-zero on error.
 */
inline int Semaphore::release(void)
{
	if (m_sem != 0)
		return -EBADF;

	// TODO: What error to return?
	return semaphore_signal(m_sem);
}

}
