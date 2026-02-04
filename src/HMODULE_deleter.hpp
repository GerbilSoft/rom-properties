/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * HMODULE_deleter.hpp: HMODULE deleter for std::unique_ptr<>.             *
 *                                                                         *
 * Copyright (c) 2025-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef _WIN32
// Unix dlopen()
#  include <dlfcn.h>
typedef void *HMODULE;
#else /* _WIN32 */
// Windows LoadLibrary()
#  include "libwin32common/RpWin32_sdk.h"
#  define dlsym(handle, symbol)	((void*)GetProcAddress(handle, symbol))
#  define dlclose(handle)	FreeLibrary(handle)
#endif /* _WIN32 */

#ifdef __cplusplus

class HMODULE_deleter
{
public:
	typedef HMODULE pointer;

	void operator()(HMODULE hModule)
	{
		if (hModule) {
			dlclose(hModule);
		}
	}
};

#endif /* __cplusplus */
