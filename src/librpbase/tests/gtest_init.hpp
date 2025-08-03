/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.hpp: Google Test initialization.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpsecure.h"
#include "gtest/gtest.h"
#include "tcharx.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SECCOMP

// Each unit test has to specify which set of syscalls are needed.
// The base set is always included.
// NOTE: This is a bitfield.
extern const unsigned int rp_gtest_syscall_set;

// Syscall sets
typedef enum {
	RP_GTEST_SYSCALL_SET_GTEST_DEATH_TEST	= (1U << 0),
	RP_GTEST_SYSCALL_SET_QT			= (1U << 1),
	RP_GTEST_SYSCALL_SET_GTK		= (1U << 2),
} RP_GTest_Syscall_Set_e;

#endif /* HAVE_SECCOMP */

#ifdef __cplusplus
}
#endif
