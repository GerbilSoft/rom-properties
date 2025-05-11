/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvtpp.hpp: Virtual Terminal wrapper functions. (C++ wrapper)           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gsvt.h"

// C++ STL classes
#include <string>

namespace Gsvt {

class Console {
public:
	Console(gsvt_console *console);
	~Console() = default;

public:
	Console(const Console &other) = delete;
	Console &operator=(const Console &other) = delete;
	Console(Console &&other) = delete;

public:
	/** Basic functions **/

	/**
	 * Force-enable color for this console.
	 */
	void forceColorOn(void)
	{
		gsvt_force_color_on(m_console);
	}

	/**
	 * Force-disable color for this console.
	 */
	void forceColorOff(void)
	{
		gsvt_force_color_off(m_console);
	}

	/**
	 * Is this console an actual console?
	 * @return True if it is; false if it isn't.
	 */
	bool isConsole(void) const
	{
		return gsvt_is_console(m_console);
	}

	/**
	 * Does this console support ANSI escape sequences?
	 * @return True if it does; false if it doesn't.
	 */
	bool supportsAnsi(void) const
	{
		return gsvt_supports_ansi(m_console);
	}

public:
	/** stdio wrapper functions **/

	/**
	 * fwrite() wrapper function.
	 *
	 * On Windows, if using a standard Windows console and ANSI escape sequences
	 * are not supported, color will be emulated using SetConsoleTextAttribute().
	 *
	 * @param ptr Characters to write
	 * @param nmemb Number of characters to write
	 * @return Number of characters written
	 */
	size_t fwrite(const char *ptr, size_t nmemb)
	{
		return gsvt_fwrite(ptr, nmemb, m_console);
	}

	/**
	 * fputs() wrapper function.
	 *
	 * On Windows, if using a standard Windows console and ANSI escape sequences
	 * are not supported, color will be emulated using SetConsoleTextAttribute().
	 *
	 * @param s NULL-terminated string to write
	 * @return Non-negative number on success, or EOF on error.
	 */
	int fputs(const char *s)
	{
		return gsvt_fputs(s, m_console);
	}

	/**
	 * fputs() wrapper function.
	 *
	 * On Windows, if using a standard Windows console and ANSI escape sequences
	 * are not supported, color will be emulated using SetConsoleTextAttribute().
	 *
	 * @param s C++ string
	 * @return Non-negative number on success, or EOF on error.
	 */
	int fputs(const std::string &s)
	{
		// NOTE: Should return a non-negative number on success, and EOF on error.
		return static_cast<int>(gsvt_fwrite(s.data(), s.size(), m_console));
	}

	/**
	 * fflush() wrapper function for gsvt_console.
	 *
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int fflush(void)
	{
		return gsvt_fflush(m_console);
	}

public:
	/** Convenience functions **/

	/**
	 * Print a newline.
	 */
	void newline(void)
	{
		gsvt_newline(m_console);
	}

public:
	/** Color functions (NOPs if the console doesn't support color) **/

	/**
	 * Set the text color.
	 * @param color ANSI text color (8 color options)
	 * @param bold If true, enable bold rendering. (commonly rendered as "bright")
	 */
	void textColorSet8(uint8_t color, bool bold)
	{
		gsvt_text_color_set8(m_console, color, bold);
	}

	/**
	 * Reset the text color to its original value.
	 */
	void textColorReset(void)
	{
		gsvt_text_color_reset(m_console);
	}

private:
	gsvt_console *const m_console;
};

extern Console StdOut;
extern Console StdErr;

} // namespace Gsvt
