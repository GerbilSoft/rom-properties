/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * timegm.c: timegm() implementation for systems that don't have it.       *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: BSL-1.0                                        *
 ***************************************************************************/

#include "stdafx.h"

#include "time_r.h"
#include <stdint.h>


// Based on the timegm() implementation from boost-1.72.0.
// boost-1.72.0/chrono/io/time_point_io.hpp.

static inline int32_t is_leap(int32_t year)
{
	if(year % 400 == 0)
	return 1;
	if(year % 100 == 0)
	return 0;
	if(year % 4 == 0)
	return 1;
	return 0;
}

static inline int32_t days_from_0(int32_t year)
{
	year--;
	return 365 * year + (year / 400) - (year/100) + (year / 4);
}

static inline int32_t days_from_1970(int32_t year)
{
	static const int32_t days_from_0_to_1970 = days_from_0(1970);
	return days_from_0(year) - days_from_0_to_1970;
}

static inline int32_t days_from_1jan(int32_t year,int32_t month,int32_t day)
{
	static const int32_t days[2][12] =
	{
		{ 0,31,59,90,120,151,181,212,243,273,304,334},
		{ 0,31,60,91,121,152,182,213,244,274,305,335}
	};

	return days[is_leap(year)][month-1] + day - 1;
}

time_t timegm(const struct tm *t)
{
	int year = t->tm_year + 1900;
	int month = t->tm_mon;
	if(month > 11)
	{
		year += month/12;
		month %= 12;
	}
	else if(month < 0)
	{
		int years_diff = (-month + 11)/12;
		year -= years_diff;
		month+=12 * years_diff;
	}
	month++;
	int day = t->tm_mday;
	int day_of_year = days_from_1jan(year,month,day);
	int days_since_epoch = days_from_1970(year) + day_of_year ;

	time_t seconds_in_day = 3600 * 24;
	time_t result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

	return result;
}
