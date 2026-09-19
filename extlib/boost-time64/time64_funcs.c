/***************************************************************************
 * ROM Properties Page shell extension. (boost-time64)                     *
 * time64_funcs.c: 64-bit time functions for systems that don't have them. *
 *                                                                         *
 * Based on boost/chrono/io/time_point_io.hpp from boost-1.90.0.           *
 * (C) Copyright Howard Hinnant                                            *
 * (C) Copyright 2010-2011 Vicente J. Botet Escriba                        *
 * SPDX-License-Identifier: BSL-1.0                                        *
 ***************************************************************************/

#include "time_r.h"
#include <stdint.h>

#ifdef USING_INTERNAL_RP_TIMEGM

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
	// FIXME: static const initialization in C?
	//static const int32_t days_from_0_to_1970 = days_from_0(1970);
	static int32_t days_from_0_to_1970 = 0;
	if (days_from_0_to_1970 == 0) {
		days_from_0_to_1970 = days_from_0(1970);
	}
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

rp_time_t rp_timegm(struct tm const *t)
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

	rp_time_t seconds_in_day = 3600 * 24;
	rp_time_t result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

	return result;
}

#endif /* USING_INTERNAL_RP_TIMEGM */

#ifdef USING_INTERNAL_RP_GMTIME

/**
* from_ymd could be made more efficient by using a table
* day_count_table indexed by the y%400.
* This table could contain the day_count
* by*365 + by/4 - by/100 + by/400
*
* from_ymd = (by/400)*days_by_400_years+day_count_table[by%400] +
* days_in_year_before[is_leap_table[by%400]][m-1] + d;
*/
static inline unsigned days_before_years(int32_t y)
{
	return y * 365 + y / 4 - y / 100 + y / 400;
}

// Returns year/month/day triple in civil calendar
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-719468].
//template <class Int>
//constexpr
static inline void civil_from_days(int32_t z, int32_t* y, unsigned* m, unsigned* d) //BOOST_NOEXCEPT
{
#if 0
	BOOST_STATIC_ASSERT_MSG(std::numeric_limits<unsigned>::digits >= 18,
		"This algorithm has not been ported to a 16 bit unsigned integer");
	BOOST_STATIC_ASSERT_MSG(std::numeric_limits<Int>::digits >= 20,
			"This algorithm has not been ported to a 16 bit signed integer");
#endif
	z += 719468;
	const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
	const unsigned doe = (unsigned int)(z - era * 146097);			// [0, 146096]
	const unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;	// [0, 399]
	*y = (int32_t)(yoe) + era * 400;
	const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);			// [0, 365]
	const unsigned mp = (5*doy + 2)/153;					// [0, 11]
	*d = doy - (153*mp+2)/5 + 1;				// [1, 31]
	*m = mp + (mp < 10 ? 3 : -9);				// [1, 12]
	*y += (*m <= 2);
	--(*m);
}

struct tm * rp_gmtime(rp_time_t const* t, struct tm *tm)
{
	if (t==0) return 0;
	if (tm==0) return 0;

#if 0
	static  const unsigned char day_of_year_month[2][366] =
	{
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 },

		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12

	} };

	static const int32_t days_in_year_before[2][13] =
	{
		{ -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 },
		{ -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }
	};
#endif

	static const rp_time_t seconds_in_day = 3600 * 24;
	int32_t days_since_epoch = (int32_t)(*t / seconds_in_day);
	int32_t hms = (int32_t)(*t - seconds_in_day*days_since_epoch);
	if (hms < 0) {
		days_since_epoch-=1;
		hms = seconds_in_day+hms;
	}

#if 0
	int32_t x = days_since_epoch;
	int32_t y = (int32_t) ((long long) (x + 2) * 400 / 146097);
	const int32_t ym1 = y - 1;
	int32_t doy = x - days_before_years(y);
	const int32_t doy1 = x - days_before_years(ym1);
	const int32_t N = std::numeric_limits<int>::digits - 1;
	const int32_t mask1 = doy >> N; // arithmetic rshift - not portable - but nearly universal
	const int32_t mask0 = ~mask1;
	doy = (doy & mask0) | (doy1 & mask1);
	y = (y & mask0) | (ym1 & mask1);
	//y -= 32767 + 2;
	y += 70;
	tm->tm_year=y;
	const int32_t leap = is_leap(y);
	tm->tm_mon = day_of_year_month[leap][doy]-1;
	tm->tm_mday = doy - days_in_year_before[leap][tm->tm_mon] ;
#else
	int32_t y;
	unsigned m, d;
	civil_from_days(days_since_epoch, &y, &m, &d);
	tm->tm_year=y-1900; tm->tm_mon=m; tm->tm_mday=d;
#endif

	tm->tm_hour = hms / 3600;
	const int ms = hms % 3600;
	tm->tm_min = ms / 60;
	tm->tm_sec = ms % 60;

	tm->tm_isdst = -1;
	(void)mktime(tm);
	return tm;
}

#endif /* USING_INTERNAL_RP_GMTIME */
