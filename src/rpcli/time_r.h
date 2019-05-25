/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * time_r.h: Workaround for missing reentrant time functions.              *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_TIME_R_H__
#define __ROMPROPERTIES_RPCLI_TIME_R_H__

#include "config.rpcli.h"

// _POSIX_C_SOURCE is required for *_r() on MinGW-w64.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#include <time.h>

#ifndef HAVE_GMTIME_R
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
#ifdef HAVE_GMTIME_S
	return (gmtime_s(result, timep) == 0 ? result : NULL);
#else /* !HAVE_GMTIME_S */
	// cppcheck-suppress gmtimeCalled
	struct tm *tm = gmtime(timep);
	if (tm && result) {
		*result = *tm;
		return result;
	}
	return nullptr;
#endif /* GMTIME_S */
}
#endif /* HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
static inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
#ifdef HAVE_LOCALTIME_S
	return (localtime_s(result, timep) == 0 ? result : NULL);
#else /* !HAVE_LOCALTIME_S */
	// cppcheck-suppress localtimeCalled
	struct tm *tm = localtime(timep);
	if (tm && result) {
		*result = *tm;
		return result;
	}
	return nullptr;
#endif /* HAVE_LOCALTIME_S */
}
#endif /* HAVE_LOCALTIME_R */

#endif /* __ROMPROPERTIES_RPCLI_TIME_R_H__ */
