/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * http-status.h: HTTP status codes.                                       *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "http-status.h"

// C API declaration for MSVC.
// Required when using stdcall as the default calling convention.
#ifdef _MSC_VER
# define RP_C_API __cdecl
#else
# define RP_C_API
#endif

/**
 * Number of elements in an array.
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
	((int)(((sizeof(x) / sizeof(x[0]))) / \
		(size_t)(!(sizeof(x) % sizeof(x[0])))))

// HTTP status code messages.
// Reference: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes

// String buffer.
static const TCHAR httpStatusMsgs_str[] = {
	// 1xx: Informational response
	_T("Continue\0")			// 100
	_T("Switching Protocols\0")		// 101
	_T("Processing\0")			// 102
	_T("Early Hints\0")			// 103

	// 2xx: Success
	_T("OK\0")				// 200
	_T("Created\0")				// 201
	_T("Accepted\0")			// 202
	_T("Non-Authoritative Information\0")	// 203
	_T("No Content\0")			// 204
	_T("Reset Content\0")			// 205
	_T("Partial Content\0")			// 206
	_T("Multi-Status\0")			// 207
	_T("Already Reported\0")		// 208
	_T("This is fine\0")			// 218 (Apache Web Server)
	_T("IM Used\0")				// 226

	// 3xx: Redirection
	_T("Multiple Choices\0")		// 300
	_T("Moved Permanently\0")		// 301
	_T("Found\0")				// 302
	_T("See Other\0")			// 303
	_T("Not Modified\0")			// 304
	_T("Use Proxy\0")			// 305
	_T("Switch Proxy\0")			// 306
	_T("Temporary Redirect\0")		// 307
	_T("Permanent Redirect\0")		// 308

	// 4xx: Client errors
	_T("Bad Request\0")			// 400
	_T("Unauthorized\0")			// 401
	_T("Payment Required\0")		// 402
	_T("Forbidden\0")			// 403
	_T("Not Found\0")			// 404
	_T("Method Not Allowed\0")		// 405
	_T("Not Acceptable\0")			// 406
	_T("Proxy Authentication Required\0")	// 407
	_T("Request Timeout\0")			// 408
	_T("Conflict\0")			// 409
	_T("Gone\0")				// 410
	_T("Length Required\0")			// 411
	_T("Precondition Failed\0")		// 412
	_T("Payload Too Large\0")		// 413
	_T("URI Too Long\0")			// 414
	_T("Unsupported Media Type\0")		// 415
	_T("Range Not Satisfiable\0")		// 416
	_T("Expectation Failed\0")		// 417
	_T("I'm a teapot\0")			// 418 (lol)
	_T("Page Expired\0")			// 419 (Laravel Framework)
	_T("Enhance Your Calm\0")		// 420 (Twitter)
	_T("Misdirected Request\0")		// 421
	_T("Unprocessable Entity\0")		// 422
	_T("Locked\0")				// 423
	_T("Failed Dependency\0")		// 424
	_T("Too Early\0")			// 425
	_T("Upgrade Required\0")		// 426
	_T("Precondition Required\0")		// 428
	_T("Too Many Requests\0")		// 429
	_T("Request Header Fields Too Large\0")	// 430 (Shopify), 431
	_T("Blocked by Windows Parental Controls\0")	// 450 (Microsoft Windows)
	_T("Unavailable For Legal Reasons\0")	// 451
	_T("Invalid Token\0")			// 498 (Esri)
	_T("Token Required\0")			// 499 (Esri)

	// 5xx: Server errors
	_T("Internal Server Error\0")		// 500
	_T("Not Implemented\0")			// 501
	_T("Bad Gateway\0")			// 502
	_T("Service Unavailable\0")		// 503
	_T("Gateway Timeout\0")			// 504
	_T("HTTP Version Not Supported\0")	// 505
	_T("Variant Also Negotiates\0")		// 506
	_T("Insufficient Storage\0")		// 507
	_T("Loop Detected\0")			// 508
	_T("Bandwidth Limit Exceeded\0")	// 509 (Apache Web Server / cPanel)
	_T("Not Extended\0")			// 510
	_T("Network Authentication Required\0")	// 511
	_T("Invalid SSL Certificate\0")		// 526 (Cloudflare / Cloud Foundry)
	_T("Site is overloaded\0")		// 529 (Qualys)
	_T("Site is frozen\0")			// 530 (Pantheon)
	_T("Network read timeout error\0")	// 598 (Used by some HTTP proxies)

	// Might not be needed...
	 _T("\0")
};

typedef struct _HttpStatusMsg_t {
	uint16_t code;
	uint16_t offset;
} HttpStatusMsg_t;

// Offsets into the string buffer.
static const HttpStatusMsg_t httpStatusMsgs[] = {
	// 1xx: Informational response
	{100,	0},
	{101,	9},
	{102,	29},
	{103,	40},

	// 2xx: Success
	{200,	52},
	{201,	55},
	{202,	63},
	{203,	72},
	{204,	102},
	{205,	113},
	{206,	127},
	{207,	143},
	{208,	156},
	{218,	173},	// Apache Web Server
	{226,	186},

	// 3xx: Redirection
	{300,	194},
	{301,	211},
	{302,	229},
	{303,	235},
	{304,	245},
	{305,	258},
	{306,	268},
	{307,	281},
	{308,	300},

	// 4xx: Client errors
	{400,	319},
	{401,	331},
	{402,	344},
	{403,	361},
	{404,	371},
	{405,	381},
	{406,	400},
	{407,	415},
	{408,	445},
	{409,	461},
	{410,	470},
	{411,	475},
	{412,	491},
	{413,	511},
	{414,	529},
	{415,	542},
	{416,	565},
	{417,	587},
	{418,	606},	// lol
	{419,	619},	// Laravel Framework
	{420,	632},	// Twitter
	{421,	651},
	{422,	670},
	{423,	691},
	{424,	698},
	{425,	716},
	{426,	726},
	{428,	743},
	{429,	765},
	{430,	783},	// Shopify
	{431,	783},	// Same as 430, but standard
	{450,	815},	// Microsoft Windows
	{451,	852},
	{498,	882},	// Esri
	{499,	896},	// Esri

	// 5xx: Server errors
	{500,	911},
	{501,	933},
	{502,	949},
	{503,	961},
	{504,	981},
	{505,	997},
	{506,	1024},
	{507,	1048},
	{508,	1069},
	{509,	1083},	// Apache Web Server / cPanel
	{510,	1108},
	{511,	1121},
	{526,	1153},	// Cloudflare / Cloud Foundry
	{529,	1177},	// Qualys
	{530,	1196},	// Pantheon
	{598,	1211},	// Used by some HTTP proxies

	{0, 0}
};

/**
 * HttpStatusMsg_t bsearch() comparison function.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API HttpStatusMsg_t_compar(const void *a, const void *b)
{
	int code1 = ((const HttpStatusMsg_t*)a)->code;
	int code2 = ((const HttpStatusMsg_t*)b)->code;
	if (code1 < code2) return -1;
	if (code1 > code2) return 1;
	return 0;
}

/**
 * Get a string representation for an HTTP status code.
 * @param code HTTP status code.
 * @return String representation, or nullptr if not found.
 */
const TCHAR *http_status_string(int code)
{
	// Do a binary search.
	const HttpStatusMsg_t key = {code, 0};
	const HttpStatusMsg_t *const res =
		(const HttpStatusMsg_t*)bsearch(&key,
			httpStatusMsgs,
			ARRAY_SIZE(httpStatusMsgs)-1,
			sizeof(HttpStatusMsg_t),
			HttpStatusMsg_t_compar);

	// Putting static_assert() after the variable declarations
	// to prevent issues with MSVC 2010.
	static_assert(ARRAY_SIZE(httpStatusMsgs_str) == 1240, "httpStatusMsgs_str[] is the wrong size!");

	return (res ? &httpStatusMsgs_str[res->offset] : NULL);
}
