/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * wmain.c: UTF-16 to UTF-8 main() wrapper for command-line programs.      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpWin32_sdk.h"

// C includes.
#include <stdlib.h>
#include <string.h>

// C API declaration for MSVC.
// Required when using stdcall as the default calling convention.
#ifdef _MSC_VER
#  define RP_C_API __cdecl
#else
#  define RP_C_API
#endif

// TODO: envp[]?
extern int RP_C_API main(int argc, char *argv[]);
int RP_C_API wmain(int argc, wchar_t *argv[])
{
	char **u8argv;
	int i, ret;

	// NOTE: We won't use librpsecure here. librpsecure functions can be
	// called by main(), since some programs will want to enable high-security
	// mode while others won't.

	// Convert the UTF-16 arguments to UTF-8.
	// NOTE: Using WideCharToMultiByte() directly in order to
	// avoid std::string overhead.
	u8argv = malloc((argc+1)*sizeof(char*));
	if (!u8argv) {
		// Memory allocation failed.
		return EXIT_FAILURE;
	}

	u8argv[argc] = NULL;
	for (i = argc-1; i >= 0; i--) {
		int cbMbs = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
		if (cbMbs <= 0) {
			// Invalid string. Make it an empty string anyway.
			char *const empty_str = malloc(1);
			if (!empty_str) {
				// Memory allocation failed.
				return EXIT_FAILURE;
			}
			empty_str[0] = '\0';
			u8argv[i] = empty_str;
			continue;
		}

		u8argv[i] = (char*)malloc(cbMbs);
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, u8argv[i], cbMbs, NULL, NULL);
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
