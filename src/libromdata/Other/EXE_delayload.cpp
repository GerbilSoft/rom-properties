/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_delayload.cpp: DOS/Windows executable reader. (DelayLoad helper)    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: EXE_p.hpp can't be included because it includes exe_structs.h.
#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#if !defined(_MSC_VER) || !defined(XML_IS_DLL)
#  error EXE_delayload.cpp should only be enabled on MSVC builds with PugiXML enabled as a DLL.
#endif

// PugiXML
#include <pugixml.hpp>
using namespace pugi;

// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.h"

namespace LibRomData {

extern volatile bool exe_dl_nc;
volatile bool exe_dl_nc = false;

// DelayLoad test implementation.
// NOTE: PugiXML doesn't export any C functions,
// so we're implementing these manually instead
// of using RpWin32_delayload.h macros.
static LONG WINAPI DelayLoad_filter_PugiXML(DWORD exceptioncode)
{
	if (exceptioncode == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)) {
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

/**
 * Test creating an xml_document.
 * @return xml_document::empty() (should be true)
 */
static bool DoXMLDocumentTest(void)
{
	xml_document doc;
	doc.reset();
	return doc.empty();
}

/**
 * Check if PugiXML can be delay-loaded.
 * @return 0 on success; negative POSIX error code on error.
 */
int DelayLoad_test_PugiXML(void)
{
	static int success = 0;
	if (!success) {
		__try {
			// We have to create an xml_document to test the
			// DLL, but __try/__except doesn't allow us to
			// do that directly, so we'll call a function.
			exe_dl_nc = DoXMLDocumentTest();
			if (!exe_dl_nc) {
				return -ENOTSUP;
			}
		} __except (DelayLoad_filter_PugiXML(GetExceptionCode())) {
			return -ENOTSUP;
		}
		success = 1;
	}
	return 0;
}

}
