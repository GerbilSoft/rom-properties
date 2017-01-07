/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * time_r.h: Workaround for missing reentrant time functions.              *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_RPCLI_TIME_R_H__
#define __ROMPROPERTIES_RPCLI_TIME_R_H__

#include "config.rpcli.h"
#include <time.h>

#ifndef HAVE_GMTIME_R
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
#ifdef HAVE_GMTIME_S
	return (gmtime_s(result, timep) == 0 ? result : NULL);
#else /* !HAVE_GMTIME_S */
	struct tm *tm = gmtime(timep);
	if (!tm)
		return nullptr;
	*result = *tm;
	return result;
#endif /* GMTIME_S */
}
#endif /* HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
static inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
#ifdef HAVE_LOCALTIME_S
	return (localtime_s(result, timep) == 0 ? result : NULL);
#else /* !HAVE_LOCALTIME_S */
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
