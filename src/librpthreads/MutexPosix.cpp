/***************************************************************************
 * ROM Properties Page shell extension. (librpthreads)                     *
 * MutexPosix.cpp: POSIX mutex implementation.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include <pthread.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRpThreads {

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

	// Disable copy/assignment constructors.
	Mutex(const Mutex &) = delete;
	Mutex &operator=(const Mutex &) = delete;

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
};

/**
 * Create a mutex.
 */
inline Mutex::Mutex()
{
	pthread_mutex_init(&m_mutex, nullptr);
}

/**
 * Delete the mutex.
 * WARNING: Mutex MUST be unlocked!
 */
inline Mutex::~Mutex()
{
	pthread_mutex_destroy(&m_mutex);
}

/**
 * Lock the mutex.
 * If the mutex is locked, this function will block until
 * the previous locker unlocks it.
 * @return 0 on success; non-zero on error.
 */
inline int Mutex::lock(void)
{
	// TODO: What error to return?
	return pthread_mutex_lock(&m_mutex);
}

/**
 * Unlock the mutex.
 * @return 0 on success; non-zero on error.
 */
inline int Mutex::unlock(void)
{
	// TODO: What error to return?
	return pthread_mutex_unlock(&m_mutex);
}

}
