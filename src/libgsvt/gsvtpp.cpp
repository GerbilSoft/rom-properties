/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvtpp.hpp: Virtual Terminal wrapper functions. (C++ wrapper)           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gsvtpp.hpp"

struct _gsvt_console {
	FILE *stream;		// File handle, e.g. stdout or stderr
	bool is_console;	// True if this is a real console and not redirected to a file
	bool supports_ansi;	// True if the console supports ANSI escape sequences
};

namespace Gsvt {

Console StdOut(gsvt_stdout);
Console StdErr(gsvt_stderr);

Console::Console(gsvt_console *console)
	: m_console(console)
{
	// Initialize consoles.
	gsvt_init();
}

}
