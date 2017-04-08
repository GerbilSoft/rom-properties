/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Semaphore.hpp: System-specific semaphore implementation.                *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_SEMAPHORE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_SEMAPHORE_HPP__

#include "common.h"

#ifdef _WIN32
#include "RpWin32_sdk.h"
#else /* !_WIN32 */
#include <semaphore.h>
#endif

namespace LibRomData {

class Semaphore
{
	public:
		/**
		 * Create a semaphore.
		 * @param count Number of times the semaphore can be obtained before blocking.
		 */
		explicit Semaphore(int count);

		/**
		 * Delete the semaphore.
		 * WARNING: Semaphore MUST be fully released!
		 */
		~Semaphore();

	private:
		RP_DISABLE_COPY(Semaphore)

	public:
		/**
		 * Obtain the semaphore.
		 * If the semaphore is at zero, this function will block
		 * until another thread releases the semaphore.
		 * @return 0 on success; non-zero on error.
		 */
		int obtain(void);

		/**
		 * Release a lock on the semaphore.
		 * @return 0 on success; non-zero on error.
		 */
		int release(void);

	private:
#ifdef _WIN32
		HANDLE m_sem;
#else
		sem_t m_sem;
		bool m_isInit;
#endif
};

/**
 * Automatic semaphore locker/unlocker class.
 * Obtains the semaphore when created.
 * Releases the semaphore when it goes out of scope.
 */
class SemaphoreLocker
{
	public:
		explicit SemaphoreLocker(Semaphore &sem)
			: m_sem(sem)
		{
			m_sem.obtain();
		}

		~SemaphoreLocker()
		{
			m_sem.release();
		}

	private:
		RP_DISABLE_COPY(SemaphoreLocker)

	private:
		Semaphore &m_sem;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_SEMAPHORE_HPP__ */
