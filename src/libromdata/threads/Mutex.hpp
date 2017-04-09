/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Mutex.hpp: System-specific mutex implementation.                        *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MUTEX_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MUTEX_HPP__

#include "common.h"

#ifdef _WIN32
#include "RpWin32_sdk.h"
#else /* !_WIN32 */
#include <pthread.h>
#endif

namespace LibRomData {

class Mutex
{
	public:
		/**
		 * Create a mutex.
		 */
		explicit Mutex();

		/**
		 * Delete the mutex.
		 * WARNING: Mutex MUST be unlocked!
		 */
		~Mutex();

	private:
		RP_DISABLE_COPY(Mutex)

	public:
		/**
		 * Lock the mutex.
		 * If the mutex is locked, this function will block until
		 * the previous locker unlocks it.
		 * @return 0 on success; non-zero on error.
		 */
		int lock(void);

		/**
		 * Unlock the mutex.
		 * @return 0 on success; non-zero on error.
		 */
		int unlock(void);

	private:
#ifdef _WIN32
		// NOTE: Windows implementation uses critical sections,
		// since they have less overhead than mutexes.
		CRITICAL_SECTION m_criticalSection;
#else
		pthread_mutex_t m_mutex;
#endif
		bool m_isInit;
};

/**
 * Automatic mutex locker/unlocker class.
 * Locks the mutex when created.
 * Unlocks the mutex when it goes out of scope.
 */
class MutexLocker
{
	public:
		explicit MutexLocker(Mutex &mutex)
			: m_mutex(mutex)
		{
			m_mutex.lock();
		}

		~MutexLocker()
		{
			m_mutex.unlock();
		}

	private:
		RP_DISABLE_COPY(MutexLocker)

	private:
		Mutex &m_mutex;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MUTEX_HPP__ */
