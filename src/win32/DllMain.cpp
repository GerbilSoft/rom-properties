/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DllMain.cpp: DLL entry point and COM registration handler.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Based on "The Complete Idiot's Guide to Writing Shell Extensions" - Part V
// http://www.codeproject.com/Articles/463/The-Complete-Idiots-Guide-to-Writing-Shell-Exten
// Demo code was released into the public domain.

// Other references:
// - "A very simple COM server without ATL or MFC"
//   - gcc and MSVC Express do not support ATL, so we can't use ATL here.
//   - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
// - "COM in C++"
//   - http://www.codeproject.com/Articles/338268/COM-in-C

#include "stdafx.h"
#include "config.version.h"
#include "config.win32.h"

#include "RP_ClassFactory.hpp"
#include "RP_ExtractIcon.hpp"
#include "RP_ExtractImage.hpp"
#include "RP_ShellPropSheetExt.hpp"
#include "RP_ThumbnailProvider.hpp"
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
#  include "RP_PropertyStore.hpp"
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
#include "RP_ShellIconOverlayIdentifier.hpp"
#include "RP_ContextMenu.hpp"
#include "RP_XAttrView.hpp"

#include "AchWin32.hpp"
#include "LanguageComboBox.hpp"
#include "MessageWidget.hpp"
#include "OptionsMenuButton.hpp"

// rp_image backend registration
#include "librptexture/img/GdiplusHelper.hpp"
#include "librptexture/img/RpGdiplusBackend.hpp"
using namespace LibRpTexture;

// GDI+ token.
static ULONG_PTR gdipToken = 0;

#include "libi18n/config.libi18n.h"
#if defined(_MSC_VER) && defined(ENABLE_NLS)
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL1(libintl_textdomain, nullptr);
#endif /* defined(_MSC_VER) && defined(ENABLE_NLS) */

/**
 * DLL entry point.
 * @param hInstance
 * @param dwReason
 * @param lpReserved
 */
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH: {
#if !defined(_MSC_VER) || defined(_DLL)
			// Disable thread library calls, since we don't
			// care about thread attachments.
			// NOTE: On MSVC, don't do this if using the static CRT.
			// Reference: https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
			DisableThreadLibraryCalls(hInstance);
#endif /* !defined(_MSC_VER) || defined(_DLL) */
			break;
		}

		case DLL_PROCESS_DETACH:
			// DLL is being unloaded.
			// FIXME: If any of our COM objects are still referenced,
			// this won't unload stuff, and things might crash...
			// Reference: https://devblogs.microsoft.com/oldnewthing/20060920-07/?p=29663
			DllCanUnloadNow();
			break;

		default:
			break;
	}

	return TRUE;
}

/**
 * Can the DLL be unloaded?
 * @return S_OK if it can; S_FALSE if it can't.
 */
__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void)
{
	if (LibWin32Common::ComBase_isReferenced()) {
		// Referenced by a COM object.
		return S_FALSE;
	}

	if (AchWin32::instance()->isAnyPopupStillActive()) {
		// Achievement window is still visible.
		return S_FALSE;
	}

	// Unregister window classes.
	MessageWidgetUnregister();
	LanguageComboBoxUnregister();
	OptionsMenuButtonUnregister();

	// Shut down GDI+ if it was initialized.
	if (gdipToken != 0) {
		GdiplusHelper::ShutdownGDIPlus(gdipToken);
		gdipToken = 0;
	}

	// Not in use.
	return S_OK;
}

/**
 * Get a class factory to create an object of the requested type.
 * @param rclsid	[in] CLSID of the object
 * @param riid		[in] IID_IClassFactory
 * @param ppv		[out] Pointer that receives the interface pointer requested in riid
 * @return Error code.
 */
_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
	if (!ppv) {
		// Incorrect parameters.
		return E_INVALIDARG;
	}

	// Clear the interface pointer initially.
	*ppv = nullptr;

#if defined(_MSC_VER) && defined(ENABLE_NLS)
	// Delay load verification.
	if (DelayLoad_test_libintl_textdomain() != 0) {
		// Delay load failed.
		return E_UNEXPECTED;
	}
#endif /* defined(_MSC_VER) && defined(ENABLE_NLS) */

	// Initialize GDI+.
	if (gdipToken == 0) {
		gdipToken = GdiplusHelper::InitGDIPlus();
		assert(gdipToken != 0);
		if (gdipToken == 0) {
			return E_OUTOFMEMORY;
		}
	}

	// Register RpGdiplusBackend and AchWin32.
	rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#if defined(ENABLE_ACHIEVEMENTS)
	AchWin32::instance();
#endif /* ENABLE_ACHIEVEMENTS */

	// Check for supported classes.
	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
	try {
		IClassFactory *pCF = nullptr;

#define CHECK_CLSID(klass) \
		if (IsEqualCLSID(rclsid, CLSID_##klass)) { \
			/* Create a new class factory for this CLSID. */ \
			pCF = new RP_ClassFactory<klass>(); \
			if (pCF) { \
				goto got_pCF; \
			} \
		}

		CHECK_CLSID(RP_ExtractIcon)
		else CHECK_CLSID(RP_ExtractImage)
		else CHECK_CLSID(RP_ShellPropSheetExt)
		else CHECK_CLSID(RP_ThumbnailProvider)
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
		else CHECK_CLSID(RP_PropertyStore)
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
#ifdef ENABLE_OVERLAY_ICON_HANDLER
		else CHECK_CLSID(RP_ShellIconOverlayIdentifier)
#endif /* ENABLE_OVERLAY_ICON_HANDLER */
		else CHECK_CLSID(RP_ContextMenu)
		else CHECK_CLSID(RP_XAttrView)

		// Unable to find a matching class.
		*ppv = nullptr;
		return hr;

	got_pCF:
		// Found a matching class.
		hr = pCF->QueryInterface(riid, ppv);
		pCF->Release();
	} catch (const std::bad_alloc&) {
		hr = E_OUTOFMEMORY;
	}

	if (hr != S_OK) {
		// Interface not found.
		*ppv = nullptr;
	}
	return hr;
}

/**
 * Get the DLL version.
 * Reference: https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/nc-shlwapi-dllgetversionproc
 */
STDAPI DllGetVersion(_Out_ DLLVERSIONINFO2 *pdvi)
{
	if (!pdvi) {
		// Return E_POINTER since pdvi is an out param.
		// Reference: http://stackoverflow.com/questions/1426672/when-return-e-pointer-and-when-e-invalidarg
		return E_POINTER;
	}

	if (pdvi->info1.cbSize < sizeof(DLLVERSIONINFO)) {
		// Invalid struct...
		return E_INVALIDARG;
	}

	// DLLVERSIONINFO
	pdvi->info1.dwMajorVersion = RP_VERSION_MAJOR;
	pdvi->info1.dwMinorVersion = RP_VERSION_MINOR;
	pdvi->info1.dwBuildNumber = RP_VERSION_PATCH;	// Not technically a build number...
	pdvi->info1.dwPlatformID = DLLVER_PLATFORM_NT;

	if (pdvi->info1.cbSize >= sizeof(DLLVERSIONINFO2)) {
		// DLLVERSIONINFO2
		pdvi->dwFlags = 0;
		pdvi->ullVersion = MAKEDLLVERULL(
			RP_VERSION_MAJOR, RP_VERSION_MINOR,
			RP_VERSION_PATCH, RP_VERSION_DEVEL);
	}

	return S_OK;
}
