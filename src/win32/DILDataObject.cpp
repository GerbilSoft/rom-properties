/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DILDataObject.cpp: IDataObject implementation for DragImageLabel.       *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

// Reference: https://www.catch22.net/tuts/ole/implementing-idataobject/
#include "DILDataObject.hpp"

// librptexture
using namespace LibRpTexture;

// C++ STL classes
#include <array>
using std::array;

class DILDataObjectPrivate
{
public:
	explicit DILDataObjectPrivate(const LibRpTexture::rp_image_const_ptr &img)
		: img(img)
	{}

public:
	rp_image_const_ptr img;

public:
	// Supported data formats
	static const array<FORMATETC, 1> formatEtc;

	/**
	 * Check if a format is supported.
	 * @param pformatetc FORMATETC
	 * @return Index if supported; -1 if not.
	 */
	int lookupFormatEtc(_In_ const FORMATETC *pformatetc) const;

	/**
	 * Helper function to copy a string to a new HGLOBAL.
	 * @param szText
	 * @param nTextLen Text length (if -1, assume a NULL-terminated string)
	 */
	static HGLOBAL StringToHandle(LPCWSTR szText, int nTextLen = -1);

	/**
	 * Helper function to duplicate an HGLOBAL.
	 * @param hMem HGLOBAL
	 * @return Duplicated HGLOBAL
	 */
	static HGLOBAL dupGlobalMem(HGLOBAL hMem);
};

/** DILDataObjectPrivate **/

// Supported data formats
const array<FORMATETC, 1> DILDataObjectPrivate::formatEtc = {{
	// TODO: "PNG" format?
	// TODO: Actually store a bitmap. Testing with just text for now.
	{CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
}};

/**
 * Check if a format is supported.
 * @param pformatetc FORMATETC
 * @return Index if supported; -1 if not.
 */
int DILDataObjectPrivate::lookupFormatEtc(_In_ const FORMATETC *pformatetc) const
{
	if (unlikely(!pformatetc)) {
		return -1;
	}

	for (int i = 0; i < static_cast<int>(formatEtc.size()); i++) {
		const FORMATETC &fmtchk = formatEtc[i];
		if (((fmtchk.tymed & pformatetc->tymed) != 0) &&
		    fmtchk.cfFormat == pformatetc->cfFormat &&
		    fmtchk.dwAspect == pformatetc->dwAspect)
		{
			// Found a matching format.
			return i;
		}
	}

	// Not found.
	return -1;
}

/**
 * Helper function to copy a string to a new HGLOBAL.
 * @param szText
 * @param nTextLen Text length (if -1, assume a NULL-terminated string)
 */
HGLOBAL DILDataObjectPrivate::StringToHandle(LPCWSTR szText, int nTextLen)
{
	// if text length is -1 then treat as a nul-terminated string
	if (nTextLen < 0) {
		nTextLen = wcslen(szText);
	}

	// allocate and lock a global memory buffer. Make it fixed
	// data so we don't have to use GlobalLock
	wchar_t *ptr = static_cast<wchar_t*>(GlobalAlloc(GMEM_FIXED, (nTextLen + 1) * sizeof(wchar_t)));

	// copy the string into the buffer
	memcpy(ptr, szText, nTextLen * sizeof(wchar_t));
	ptr[nTextLen] = '\0';

	return ptr;
}

/**
 * Helper function to duplicate an HGLOBAL.
 * @param hMem HGLOBAL
 * @return Duplicated HGLOBAL
 */
HGLOBAL DILDataObjectPrivate::dupGlobalMem(HGLOBAL hMem)
{
	SIZE_T len = GlobalSize(hMem);
	PVOID source = GlobalLock(hMem);
	PVOID dest = GlobalAlloc(GMEM_FIXED, len);

	memcpy(dest, source, len);
	GlobalUnlock(hMem);
	return dest;
}

/** DILDataObject **/

DILDataObject::DILDataObject(const LibRpTexture::rp_image_const_ptr &img)
	: d_ptr(new DILDataObjectPrivate(img))
{}

DILDataObject::~DILDataObject()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP DILDataObject::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(DILDataObject, IDataObject),
		QITABENT(DILDataObject, IDropSource),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IDataObject **/
// Reference: https://www.catch22.net/tuts/ole/implementing-idataobject/

IFACEMETHODIMP DILDataObject::GetData(_In_ FORMATETC *pformatetcIn, _Out_ STGMEDIUM *pmedium)
{
	RP_D(const DILDataObject);
	int idx = d->lookupFormatEtc(pformatetcIn);
	if (idx < 0) {
		// Not supported...
		return DV_E_FORMATETC;
	}

	// found a match - transfer data into supplied storage medium
	const FORMATETC &fmtetc = d->formatEtc[idx];
	pmedium->tymed = fmtetc.tymed;
	pmedium->pUnkForRelease = nullptr;

	switch(fmtetc.tymed)
	{
		case TYMED_HGLOBAL:
			// TODO: Prepare the image data.
			// For now, just creating a string.
			//pmedium->hGlobal = d->dupGlobalMem(d->stgMedium[idx].hGlobal);
			pmedium->hGlobal = d->StringToHandle(L"Hello, World!");
			break;

		default:
			return DV_E_FORMATETC;
	}

	return S_OK;
}

IFACEMETHODIMP DILDataObject::GetDataHere(_In_ FORMATETC *pformatetc, _Inout_ STGMEDIUM *pmedium)
{
	// Only HGLOBAL (in-memory) data is supported,
	// e.g. from an rp_image or IconAnimData.
	RP_UNUSED(pformatetc);
	RP_UNUSED(pmedium);
	return DATA_E_FORMATETC;
}

IFACEMETHODIMP DILDataObject::QueryGetData(__RPC__in_opt FORMATETC *pformatetc)
{
	RP_D(const DILDataObject);
	return (d->lookupFormatEtc(pformatetc) == -1) ? DV_E_FORMATETC : S_OK;
}

IFACEMETHODIMP DILDataObject::GetCanonicalFormatEtc(__RPC__in_opt FORMATETC *pformatetcIn, __RPC__out FORMATETC *pformatetcOut)
{
	// Not needed.
	RP_UNUSED(pformatetcIn);
	if (!pformatetcOut) {
		return E_POINTER;
	}
	pformatetcOut->ptd = nullptr;
	return E_NOTIMPL;
}

IFACEMETHODIMP DILDataObject::SetData(_In_ FORMATETC *pformatetc, _In_ STGMEDIUM *pmedium, BOOL fRelease)
{
	// Not supported; data can only be set from the constructor.
	RP_UNUSED(pformatetc);
	RP_UNUSED(fRelease);
	return E_NOTIMPL;
}

IFACEMETHODIMP DILDataObject::EnumFormatEtc(DWORD dwDirection, __RPC__deref_out_opt IEnumFORMATETC **ppenumFormatEtc)
{
	if (unlikely(!ppenumFormatEtc)) {
		return E_POINTER;
	}

	// Only GET is supported.
	if (dwDirection != DATADIR_GET) {
		return E_NOTIMPL;
	}

	// Create an IEnumFORMATETC.
	// NOTE: Win2000+ has SHCreateStdEnumFmtEtc(), but it's not available in older versions.
	// For 9x compatibility, this can be implemented later.
	// Reference: https://www.catch22.net/tuts/ole/enumerating-formatetc/
	HRESULT hr = SHCreateStdEnumFmtEtc(
		static_cast<int>(DILDataObjectPrivate::formatEtc.size()),
		DILDataObjectPrivate::formatEtc.data(),
		ppenumFormatEtc);
	if (FAILED(hr) || !*ppenumFormatEtc) {
		if (*ppenumFormatEtc) {
			(*ppenumFormatEtc)->Release();
		}
		*ppenumFormatEtc = nullptr;

		if (!FAILED(hr)) {
			hr = E_FAIL;
		}
	}

	return hr;
}

IFACEMETHODIMP DILDataObject::DAdvise(__RPC__in FORMATETC *pformatetc, DWORD advf, __RPC__in_opt IAdviseSink *pAdvSink, __RPC__out DWORD *pdwConnection)
{
	// Not supported.
	RP_UNUSED(pformatetc);
	RP_UNUSED(advf);
	RP_UNUSED(pAdvSink);
	RP_UNUSED(pdwConnection);
	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP DILDataObject::DUnadvise(DWORD dwConnection)
{
	// Not supported.
	RP_UNUSED(dwConnection);
	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP DILDataObject::EnumDAdvise(__RPC__deref_out_opt IEnumSTATDATA **ppenumAdvise)
{
	// Not supported.
	RP_UNUSED(ppenumAdvise);
	return OLE_E_ADVISENOTSUPPORTED;
}

/** IDropSource **/

IFACEMETHODIMP DILDataObject::QueryContinueDrag(_In_ BOOL fEscapePressed, _In_ DWORD grfKeyState)
{
	// if the Escape key has been pressed since the last call, cancel the drop
	if (fEscapePressed) {
		return DRAGDROP_S_CANCEL;
	}

	// if the LeftMouse button has been released, then do the drop!
	if ((grfKeyState & MK_LBUTTON) == 0) {
		return DRAGDROP_S_DROP;
	}

	// continue with the drag-drop
	return S_OK;
}

IFACEMETHODIMP DILDataObject::GiveFeedback(_In_ DWORD dwEffect)
{
	// TODO: Do something here?
	RP_UNUSED(dwEffect);
	return DRAGDROP_S_USEDEFAULTCURSORS;
}
