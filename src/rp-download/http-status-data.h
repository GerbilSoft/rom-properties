/** http_status (generated from http-status-data.txt) **/
#pragma once

#include <stdint.h>

static const TCHAR http_status_strtbl[] =
	_T("\x00") _T("Continue") _T("\x00") _T("Switching Protocols") _T("\x00") _T("Processing")
	_T("\x00") _T("Early Hints") _T("\x00") _T("OK") _T("\x00") _T("Created") _T("\x00") _T("Accepte")
	_T("d") _T("\x00") _T("Non-Authoritative Information") _T("\x00") _T("No Content") _T("\x00")
	_T("Reset Content") _T("\x00") _T("Partial Content") _T("\x00") _T("Multi-Status") _T("\x00")
	_T("Already Reported") _T("\x00") _T("This is fine") _T("\x00") _T("IM Used") _T("\x00") _T("M")
	_T("ultiple Choices") _T("\x00") _T("Moved Permanently") _T("\x00") _T("Found") _T("\x00")
	_T("See Other") _T("\x00") _T("Not Modified") _T("\x00") _T("Use Proxy") _T("\x00") _T("Swit")
	_T("ch Proxy") _T("\x00") _T("Temporary Redirect") _T("\x00") _T("Permanent Redirect")
	_T("\x00") _T("Bad Request") _T("\x00") _T("Unauthorized") _T("\x00") _T("Payment Requir")
	_T("ed") _T("\x00") _T("Forbidden") _T("\x00") _T("Not Found") _T("\x00") _T("Method Not All")
	_T("owed") _T("\x00") _T("Not Acceptable") _T("\x00") _T("Proxy Authentication Requi")
	_T("red") _T("\x00") _T("Request Timeout") _T("\x00") _T("Conflict") _T("\x00") _T("Gone") _T("\x00")
	_T("Length Required") _T("\x00") _T("Precondition Failed") _T("\x00") _T("Payload To")
	_T("o Large") _T("\x00") _T("URI Too Long") _T("\x00") _T("Unsupported Media Type") _T("\x00")
	_T("Range Not Satisfiable") _T("\x00") _T("Expectation Failed") _T("\x00") _T("I'm a")
	_T(" teapot") _T("\x00") _T("Page Expired") _T("\x00") _T("Enhance Your Calm") _T("\x00") _T("M")
	_T("isdirected Request") _T("\x00") _T("Unprocessable Entity") _T("\x00") _T("Locked")
	_T("\x00") _T("Failed Dependency") _T("\x00") _T("Too Early") _T("\x00") _T("Upgrade Req")
	_T("uired") _T("\x00") _T("Precondition Required") _T("\x00") _T("Too Many Requests") _T("\x00")
	_T("Request Header Fields Too Large") _T("\x00") _T("Blocked by Windows Pare")
	_T("ntal Controls") _T("\x00") _T("Unavailable For Legal Reasons") _T("\x00") _T("In")
	_T("valid Token") _T("\x00") _T("Token Required") _T("\x00") _T("Internal Server Err")
	_T("or") _T("\x00") _T("Not Implemented") _T("\x00") _T("Bad Gateway") _T("\x00") _T("Servic")
	_T("e Unavailable") _T("\x00") _T("Gateway Timeout") _T("\x00") _T("HTTP Version Not")
	_T(" Supported") _T("\x00") _T("Variant Also Negotiates") _T("\x00") _T("Insufficien")
	_T("t Storage") _T("\x00") _T("Loop Detected") _T("\x00") _T("Bandwidth Limit Exceed")
	_T("ed") _T("\x00") _T("Not Extended") _T("\x00") _T("Network Authentication Require")
	_T("d") _T("\x00") _T("Web Server returns an unknown error") _T("\x00") _T("Web Serv")
	_T("er is down") _T("\x00") _T("Connection timed out") _T("\x00") _T("Origin is unre")
	_T("achable") _T("\x00") _T("A timeout occurred") _T("\x00") _T("SSL handshake faile")
	_T("d") _T("\x00") _T("Invalid SSL Certificate") _T("\x00") _T("Railgun Listener to ")
	_T("origin error") _T("\x00") _T("Site is overloaded") _T("\x00") _T("Site is frozen")
	_T("\x00") _T("Network read timeout error") _T("\x00");

typedef struct _HttpStatusMsg_t {
	uint16_t code;
	uint16_t offset;
} HttpStatusMsg_t;

static const HttpStatusMsg_t http_status_offtbl[] = {
	{100, 1},
	{101, 10},
	{102, 30},
	{103, 41},

	{200, 53},
	{201, 56},
	{202, 64},
	{203, 73},
	{204, 103},
	{205, 114},
	{206, 128},
	{207, 144},
	{208, 157},
	{218, 174},
	{226, 187},

	{300, 195},
	{301, 212},
	{302, 230},
	{303, 236},
	{304, 246},
	{305, 259},
	{306, 269},
	{307, 282},
	{308, 301},

	{400, 320},
	{401, 332},
	{402, 345},
	{403, 362},
	{404, 372},
	{405, 382},
	{406, 401},
	{407, 416},
	{408, 446},
	{409, 462},
	{410, 471},
	{411, 476},
	{412, 492},
	{413, 512},
	{414, 530},
	{415, 543},
	{416, 566},
	{417, 588},
	{418, 607},
	{419, 620},
	{420, 633},
	{421, 651},
	{422, 671},
	{423, 692},
	{424, 699},
	{425, 717},
	{426, 727},
	{428, 744},
	{429, 766},
	{430, 784},
	{431, 784},
	{450, 816},
	{451, 853},
	{498, 883},
	{499, 897},

	{500, 912},
	{501, 934},
	{502, 950},
	{503, 962},
	{504, 982},
	{505, 998},
	{506, 1025},
	{507, 1049},
	{508, 1070},
	{509, 1084},
	{510, 1109},
	{511, 1122},
	{520, 1154},
	{521, 1190},
	{522, 1209},
	{523, 1230},
	{524, 1252},
	{525, 1271},
	{526, 1292},
	{527, 1316},
	{529, 1349},
	{530, 1368},
	{598, 1383},
};
