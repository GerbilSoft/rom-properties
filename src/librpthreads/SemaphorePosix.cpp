/***************************************************************************
 * ROM Properties Page shell extension. (librpthreads)                     *
 * SemaphorePosix.cpp: POSIX semaphore implementation.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include <semaphore.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRpThreads {

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

	// Disable copy/assignment constructors.
	Semaphore(const Semaphore &) = delete;
	Semaphore &operator=(const Semaphore &) = delete;

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
	sem_t m_sem;
	bool m_isInit;
};

/**
 * Create a semaphore.
 * @param count Number of times the semaphore can be obtained before blocking.
 */
inline Semaphore::Semaphore(int count)
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
inline Semaphore::~Semaphore()
{
	if (m_isInit) {
		// TODO: Error checking.
		sem_destroy(&m_sem);
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
	if (!m_isInit)
		return -EBADF;

	// TODO: What error to return?
	return sem_wait(&m_sem);
}

/**
 * Release a lock on the semaphore.
 * @return 0 on success; non-zero on error.
 */
inline int Semaphore::release(void)
{
	if (!m_isInit)
		return -EBADF;

	// TODO: What error to return?
	return sem_post(&m_sem);
}

}
