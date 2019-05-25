/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Mutex.hpp: System-specific mutex implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_MUTEX_HPP__
#define __ROMPROPERTIES_LIBRPBASE_MUTEX_HPP__

#include "common.h"

// NOTE: The .cpp files are #included here in order to inline the functions.
// Do NOT compile them separately!

// Each .cpp file defines the Mutex class itself, with required fields.

#ifdef _WIN32
# include "MutexWin32.cpp"
#else /* !_WIN32 */
# include "MutexPosix.cpp"
#endif

namespace LibRpBase {

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
		RP_DISABLE_COPY(MutexLocker)

	private:
		Mutex &m_mutex;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_MUTEX_HPP__ */
