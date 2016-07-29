// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ExtractIcon.hpp"

// CLSID
const GUID CLSID_RP_ExtractIcon =
	{0xe51bc107, 0xe491, 0x4b29, {0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02}};
const wchar_t CLSID_RP_ExtractIcon_Str[] =
	L"{E51BC107-E491-4B29-A6A3-2A4309259802}";

RP_ExtractIcon::RP_ExtractIcon()
{ }

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

STDMETHODIMP RP_ExtractIcon::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IExtractIcon) {
		*ppvObj = static_cast<IExtractIcon*>(this);
	} else if (riid == IID_IPersistFile) {
		*ppvObj = static_cast<IPersistFile*>(this);
	} else {
		// Interface is not supported.
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/** IExtractIcon **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761854(v=vs.85).aspx

STDMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	__out_ecount(cchMax) LPTSTR pszIconFile, UINT cchMax,
	__out int *piIndex, __out UINT *pwFlags)
{
	// TODO: If the icon is cached on disk, return a filename.
	*pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
	return S_OK;
}

STDMETHODIMP RP_ExtractIcon::Extract(LPCTSTR pszFile, UINT nIconIndex,
	__out_opt HICON *phiconLarge, __out_opt HICON *phiconSmall,
	UINT nIconSize)
{
	// TODO: Extract the actual icon.
	// For now, create an icon that's all cyan.
	// Reference: http://stackoverflow.com/questions/7375003/how-to-convert-hicon-to-hbitmap-in-vc
	HDC hDC = GetDC(NULL);
	HDC hMemDC = CreateCompatibleDC(hDC);
	HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, 32, 32);
	HANDLE hPrevObj = SelectObject(hMemDC, hMemBmp);

	HBRUSH hCyanBrush = CreateSolidBrush(RGB(0, 255, 255));
	RECT rect = {0, 0, 32, 32};
	FillRect(hMemDC, &rect, hCyanBrush);
	SelectObject(hMemDC, hPrevObj);

	// Create a mask.
	HBITMAP hbmMask = CreateCompatibleBitmap(hMemDC, 0, 0);

	// Convert to an icon.
	// Reference: http://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP&p=1661856#post1661856
	ICONINFO ii = {0};
	ii.fIcon = TRUE;
	ii.hbmColor = hMemBmp;
	ii.hbmMask = hbmMask;

	HICON hIcon = CreateIconIndirect(&ii);

	// Clean up.
	// TODO: DeleteBrush() et al?
	DeleteObject(hMemDC);
	DeleteObject(hMemBmp);
	DeleteObject(hCyanBrush);
	DeleteObject(hbmMask);
	ReleaseDC(NULL, hDC);

	if (phiconLarge) {
		*phiconLarge = hIcon;
	} else {
		DeleteObject(hIcon);
	}

	if (phiconSmall) {
		// FIXME: Is this valid?
		*phiconSmall = nullptr;
	}

	return S_OK;
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

STDMETHODIMP RP_ExtractIcon::GetClassID(__RPC__out CLSID *pClassID)
{
	// 3DS extension returns E_NOTIMPL here.
	// TODO: Set a CLSID for RP_ExtractIcon?
	// TODO: Use separate CLSIDs for the different RP_ classes,
	// instead of one CLSID for everything?
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::IsDirty(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::Load(__RPC__in LPCOLESTR pszFileName, DWORD dwMode)
{
	// pszFileName is the file being worked on.
	// TODO: Do we need that for anything?
	return S_OK;
}

STDMETHODIMP RP_ExtractIcon::Save(__RPC__in_opt LPCOLESTR pszFileName, BOOL fRemember)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::SaveCompleted(__RPC__in_opt LPCOLESTR pszFileName)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::GetCurFile(__RPC__deref_out_opt LPOLESTR *ppszFileName)
{
	return E_NOTIMPL;
}
