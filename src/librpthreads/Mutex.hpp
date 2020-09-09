/***************************************************************************
 * ROM Properties Page shell extension. (librpthreads)                     *
 * Mutex.hpp: System-specific mutex implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTHREADS_MUTEX_HPP__
#define __ROMPROPERTIES_LIBRPTHREADS_MUTEX_HPP__

// NOTE: The .cpp files are #included here in order to inline the functions.
// Do NOT compile them separately!

// Each .cpp file defines the Mutex class itself, with required fields.

#ifdef _WIN32
# include "MutexWin32.cpp"
#else /* !_WIN32 */
# include "MutexPosix.cpp"
#endif

namespace LibRpThreads {

/**
 * Automatic mutex locker/unlocker class.
 * Locks the mutex when created.
 * Unlocks the mutex when it goes out of scope.
 */
class MutexLocker
{
	public:
		inline explicit MutexLocker(Mutex &mutex)
			: m_mutex(mutex)
		{
			m_mutex.lock();
		}

		inline ~MutexLocker()
		{
			m_mutex.unlock();
		}

	private:
#if __cplusplus >= 201103L
		MutexLocker(const MutexLocker &) = delete; \
		MutexLocker &operator=(const MutexLocker &) = delete;
#else /* __cplusplus < 201103L */
		MutexLocker(const MutexLocker &); \
		MutexLocker &operator=(const MutexLocker &);
#endif /* __cplusplus */

	private:
		Mutex &m_mutex;
};

}

#endif /* __ROMPROPERTIES_LIBRPTHREADS_MUTEX_HPP__ */
