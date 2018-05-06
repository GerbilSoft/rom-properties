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
#include <dispatch/dispatch.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// NOTE: GCD was introduced with Mac OS X 10.6.
// TODO: Get an unnamed semaphore implementation that works on earlier versions.
// POSIX semaphore functions are defined, but return ENOSYS.
// References:
// - https://stackoverflow.com/questions/27736618/why-are-sem-init-sem-getvalue-sem-destroy-deprecated-on-mac-os-x-and-w
// - https://stackoverflow.com/a/27847103

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
		dispatch_semaphore_t m_sem;
};

/**
 * Create a semaphore.
 * @param count Number of times the semaphore can be obtained before blocking.
 */
inline Semaphore::Semaphore(int count)
	: m_sem(nullptr)
{
	m_sem = dispatch_semaphore_create(count);
	assert(m_sem != nullptr);
	// FIXME: Do something if an error occurred here...
}

/**
 * Delete the semaphore.
 * WARNING: Semaphore MUST be fully released!
 */
inline Semaphore::~Semaphore()
{
	if (m_sem) {
		dispatch_release(m_sem);
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
	if (!m_sem)
		return -EBADF;

	return dispatch_semaphore_wait(m_sem, DISPATCH_TIME_FOREVER);
}

/**
 * Release a lock on the semaphore.
 * @return 0 on success; non-zero on error.
 */
inline int Semaphore::release(void)
{
	if (!m_sem)
		return -EBADF;

	// TODO: What error to return?
	return dispatch_semaphore_signal(m_sem);
}

}
