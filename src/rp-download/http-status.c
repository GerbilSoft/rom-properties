/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * http-status.h: HTTP status codes.                                       *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "http-status.h"

// C includes.
#include <stdlib.h>

// C API declaration for MSVC.
// Required when using stdcall as the default calling convention.
#ifdef _MSC_VER
# define RP_C_API __cdecl
#else
# define RP_C_API
#endif

typedef struct _HttpStatusMsg_t {
	int code;
	const TCHAR *msg;
} HttpStatusMsg_t;

// HTTP status code messages.
// Reference: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
// TODO: Use a single string buffer and offsets instead of string pointers.
static const HttpStatusMsg_t httpStatusMsgs[] = {
	// 1xx: Informational response
	{100,	_T("Continue")},
	{101,	_T("Switching Protocols")},
	{102,	_T("Processing")},
	{103,	_T("Early Hints")},

	// 2xx: Success
	{200,	_T("OK")},
	{201,	_T("Created")},
	{202,	_T("Accepted")},
	{203,	_T("Non-Authoritative Information")},
	{204,	_T("No Content")},
	{205,	_T("Reset Content")},
	{206,	_T("Partial Content")},
	{207,	_T("Multi-Status")},
	{208,	_T("Already Reported")},
	{226,	_T("IM Used")},

	// 3xx: Redirection
	{300,	_T("Multiple Choices")},
	{301,	_T("Moved Permanently")},
	{302,	_T("Found")},
	{303,	_T("See Other")},
	{304,	_T("Not Modified")},
	{305,	_T("Use Proxy")},
	{306,	_T("Switch Proxy")},
	{307,	_T("Temporary Redirect")},
	{308,	_T("Permanent Redirect")},

	// 4xx: Client errors
	{400,	_T("Bad Request")},
	{401,	_T("Unauthorized")},
	{402,	_T("Payment Required")},
	{403,	_T("Forbidden")},
	{404,	_T("Not Found")},
	{405,	_T("Method Not Allowed")},
	{406,	_T("Not Acceptable")},
	{407,	_T("Proxy Authentication Required")},
	{408,	_T("Request Timeout")},
	{409,	_T("Conflict")},
	{410,	_T("Gone")},
	{411,	_T("Length Required")},
	{412,	_T("Precondition Failed")},
	{413,	_T("Payload Too Large")},
	{414,	_T("URI Too Long")},
	{415,	_T("Unsupported Media Type")},
	{416,	_T("Range Not Satisfiable")},
	{417,	_T("Expectation Failed")},
	{418,	_T("I'm a teapot")},	// lol
	{421,	_T("Misdirected Request")},
	{422,	_T("Unprocessable Entity")},
	{423,	_T("Locked")},
	{424,	_T("Failed Dependency")},
	{425,	_T("Too Early")},
	{426,	_T("Upgrade Required")},
	{428,	_T("Precondition Required")},
	{429,	_T("Too Many Requests")},
	{431,	_T("Request Header Fields Too Large")},
	{451,	_T("Unavailable For Legal Reasons")},

	// 5xx: Server errors
	{500,	_T("Internal Server Error")},
	{501,	_T("Not Implemented")},
	{502,	_T("Bad Gateway")},
	{503,	_T("Service Unavailable")},
	{504,	_T("Gateway Timeout")},
	{505,	_T("HTTP Version Not Supported")},
	{506,	_T("Variant Also Negotiates")},
	{507,	_T("Insufficient Storage")},
	{508,	_T("Loop Detected")},
	{510,	_T("Not Extended")},
	{511,	_T("Network Authentication Required")},
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
	const HttpStatusMsg_t key = {code, NULL};
	const HttpStatusMsg_t *res =
		(const HttpStatusMsg_t*)bsearch(&key,
			httpStatusMsgs,
			sizeof(httpStatusMsgs)/sizeof(httpStatusMsgs[0]),
			sizeof(HttpStatusMsg_t),
			HttpStatusMsg_t_compar);
	return (res ? res->msg : NULL);
}
