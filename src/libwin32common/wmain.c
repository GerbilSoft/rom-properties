/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * wmain.c: UTF-16 to UTF-8 main() wrapper for command-line programs.      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "secoptions.h"
#include "RpWin32_sdk.h"

// C includes.
#include <stdlib.h>

// TODO: envp[]?
extern int main(int argc, char *argv[]);
int wmain(int argc, wchar_t *argv[])
{
	char **u8argv;
	int i, ret;

	// Set Win32 security options.
	secoptions_init();

	// Convert the UTF-16 arguments to UTF-8.
	// NOTE: Using WideCharToMultiByte() directly in order to
	// avoid std::string overhead.
	u8argv = malloc((argc+1)*sizeof(char*));
	u8argv[argc] = nullptr;
	for (i = 0; i < argc; i++) {
		int cbMbs = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		if (cbMbs <= 0) {
			// Invalid string. Make it an empty string anyway.
			u8argv[i] = strdup("");
			continue;
		}

		u8argv[i] = (char*)malloc(cbMbs);
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, u8argv[i], cbMbs, nullptr, nullptr);
	}

	// Run the program.
	ret = main(argc, u8argv);

	// Free u8argv[].
	for (i = argc-1; i >= 0; i--) {
		free(u8argv[i]);
	}
	free(u8argv);

	return ret;
}
