/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_delayload.cpp: DOS/Windows executable reader. (DelayLoad helper)    *
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

// NOTE: EXE_p.hpp can't be included because it includes exe_structs.h.
#include "librpbase/config.librpbase.h"
#if !defined(_MSC_VER) || !defined(XML_IS_DLL)
#error EXE_delayload.cpp should only be enabled on MSVC builds with TinyXML2 enabled as a DLL.
#endif

// TinyXML2
#include "tinyxml2.h"
using namespace tinyxml2;

// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.hpp"

namespace LibRomData {

extern volatile Whitespace exe_dl_ws;
volatile Whitespace exe_dl_ws = PRESERVE_WHITESPACE;

// DelayLoad test implementation.
// NOTE: TinyXML2 doesn't export any C functions,
// so we're implementing these manually instead
// of using RpWin32_delayload.h macros.
static LONG WINAPI DelayLoad_filter_TinyXML2(DWORD exceptioncode)
{
	if (exceptioncode == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)) {
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

/**
 * Check if TinyXML2 can be delay-loaded.
 * @return 0 on success; negative POSIX error code on error.
 */
int DelayLoad_test_TinyXML2(void)
{
	static int success = 0;
	volatile char tmp[sizeof(XMLDocument)] = { 0 };
	if (!success) {
		__try {
			// XMLDocument::WhitespaceMode() is non-virtual and
			// returns a single value from within the struct.
			// It should work with our pseudo-struct.
			exe_dl_ws = ((const XMLDocument*)&tmp[0])->WhitespaceMode();
		} __except (DelayLoad_filter_TinyXML2(GetExceptionCode())) {
			return -ENOTSUP;
		}
		success = 1;
	}
	return 0;
}

}
