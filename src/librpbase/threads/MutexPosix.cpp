/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * MutexPosix.cpp: POSIX mutex implementation.                             *
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

#include "Mutex.hpp"
#include <pthread.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRpBase {

class Mutex
{
	public:
		/**
		 * Create a mutex.
		 */
		inline explicit Mutex();

		/**
		 * Delete the mutex.
		 * WARNING: Mutex MUST be unlocked!
		 */
		inline ~Mutex();

	private:
		RP_DISABLE_COPY(Mutex)

	public:
		/**
		 * Lock the mutex.
		 * If the mutex is locked, this function will block until
		 * the previous locker unlocks it.
		 * @return 0 on success; non-zero on error.
		 */
		inline int lock(void);

		/**
		 * Unlock the mutex.
		 * @return 0 on success; non-zero on error.
		 */
		inline int unlock(void);

	private:
		pthread_mutex_t m_mutex;
		bool m_isInit;
};

/**
 * Create a mutex.
 */
inline Mutex::Mutex()
	: m_isInit(false)
{
	int ret = pthread_mutex_init(&m_mutex, nullptr);
	assert(ret == 0);
	if (ret == 0) {
		m_isInit = true;
	} else {
		// FIXME: Do something if an error occurred here...
	}
}

/**
 * Delete the mutex.
 * WARNING: Mutex MUST be unlocked!
 */
inline Mutex::~Mutex()
{
	if (m_isInit) {
		// TODO: Error checking.
		pthread_mutex_destroy(&m_mutex);
	}
}

/**
 * Lock the mutex.
 * If the mutex is locked, this function will block until
 * the previous locker unlocks it.
 * @return 0 on success; non-zero on error.
 */
inline int Mutex::lock(void)
{
	if (!m_isInit)
		return -EBADF;

	// TODO: What error to return?
	return pthread_mutex_lock(&m_mutex);
}

/**
 * Unlock the mutex.
 * @return 0 on success; non-zero on error.
 */
inline int Mutex::unlock(void)
{
	if (!m_isInit)
		return -EBADF;

	// TODO: What error to return?
	return pthread_mutex_unlock(&m_mutex);
}

}
