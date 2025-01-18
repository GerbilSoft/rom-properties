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
