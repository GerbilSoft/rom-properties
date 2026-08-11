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
#include "RpImageWin32.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPngWriter.hpp"
#include "librpfile/VectorFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpTexture;

// libwin32ui
#include "libwin32ui/WinUI.hpp"
#include <uxtheme.h>	// for IsThemeActive()

// C++ STL classes
#include <array>
#include <memory>
using std::array;
using std::unique_ptr;

#ifdef HAVE_STD_VARIANT
#  include <variant>
#else /* !HAVE_STD_VARIANT */
// std::variant<> is not available on this system.
// Use mpark variant instead.
#  include "mpark/variant.hpp"
namespace std {
	using mpark::variant;
	using mpark::holds_alternative;
	using mpark::get;
	using mpark::monostate;
}
#endif /* HAVE_STD_VARIANT */

class DILDataObjectPrivate
{
public:
	explicit DILDataObjectPrivate(const rp_image_const_ptr &img);
	explicit DILDataObjectPrivate(const IconAnimDataConstPtr &iconAnimData);
private:
	/**
	 * Initialize the formatEtc array.
	 */
	void initFormatEtc(void);

public:
	std::variant<std::monostate, rp_image_const_ptr, IconAnimDataConstPtr> imgData;

public:
	// Supported data formats
	array<FORMATETC, 3> formatEtc;

	/**
	 * Check if a format is supported.
	 * @param pformatetc FORMATETC
	 * @return Index if supported; -1 if not.
	 */
	int lookupFormatEtc(_In_ const FORMATETC *pformatetc) const;

	/**
	 * Get the CFSTR_FILEDESCRIPTORW for the image.
	 * @return CFSTR_FILEDESCRIPTORW
	 */
	HGLOBAL getFileDescriptorW(void) const;

	/**
	 * Get the CFSTR_FILEDESCRIPTORA for the image.
	 * @return CFSTR_FILEDESCRIPTORA
	 */
	HGLOBAL getFileDescriptorA(void) const;

	/**
	 * Get the image data in PNG format.
	 * @return HGLOBAL containing the image data, or nullptr on error.
	 */
	HGLOBAL getFileContents(void) const;
};

/** DILDataObjectPrivate **/

DILDataObjectPrivate::DILDataObjectPrivate(const rp_image_const_ptr &img)
	: imgData(img)
{
	initFormatEtc();
}

DILDataObjectPrivate::DILDataObjectPrivate(const IconAnimDataConstPtr &iconAnimData)
	: imgData(iconAnimData)
{
	initFormatEtc();
}

/**
 * Initialize the formatEtc array.
 */
void DILDataObjectPrivate::initFormatEtc(void)
{
	// Initialize formatEtc.
	// NOTE: Cannot make it static const because CFSTR_* formats have to be
	// converted to integers using RegisterClipboardFormat().

	// CFSTR_FILEDESCRIPTORW
	formatEtc[0].cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
	formatEtc[0].ptd = nullptr;
	formatEtc[0].dwAspect = DVASPECT_CONTENT;
	formatEtc[0].lindex = -1;
	formatEtc[0].tymed = TYMED_HGLOBAL;

	// CFSTR_FILEDESCRIPTORA
	formatEtc[1].cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
	formatEtc[1].ptd = nullptr;
	formatEtc[1].dwAspect = DVASPECT_CONTENT;
	formatEtc[1].lindex = -1;
	formatEtc[1].tymed = TYMED_HGLOBAL;

	// CFSTR_FILECONTENTS
	formatEtc[2].cfFormat = RegisterClipboardFormat(CFSTR_FILECONTENTS);
	formatEtc[2].ptd = nullptr;
	formatEtc[2].dwAspect = DVASPECT_CONTENT;
	formatEtc[2].lindex = 0;
	formatEtc[2].tymed = TYMED_HGLOBAL;
}

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
 * Get the CFSTR_FILEDESCRIPTORW for the image.
 * @return CFSTR_FILEDESCRIPTORW
 */
HGLOBAL DILDataObjectPrivate::getFileDescriptorW(void) const
{
	// Create a CFSTR_FILEDESCRIPTOR for a single virtual file.
	// NOTE: FILEGROUPDESCRIPTORW contains one FILEDESCRIPTORW.
	const size_t buf_size = sizeof(FILEGROUPDESCRIPTORW);
	HGLOBAL hglbFileDesc = GlobalAlloc(GMEM_MOVEABLE, buf_size);
	if (!hglbFileDesc) {
		return nullptr;
	}

	FILEGROUPDESCRIPTORW *const fileGroupDesc = static_cast<FILEGROUPDESCRIPTORW*>(GlobalLock(hglbFileDesc));
	if (!fileGroupDesc) {
		GlobalFree(hglbFileDesc);
		return nullptr;
	}

	fileGroupDesc->cItems = 1;

	FILEDESCRIPTORW *const fileDesc = &fileGroupDesc->fgd[0];
	fileDesc->dwFlags = FD_ATTRIBUTES;
	fileDesc->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	wcscpy(fileDesc->cFileName, L"🍅.png");

	GlobalUnlock(hglbFileDesc);
	return hglbFileDesc;
}

/**
 * Get the CFSTR_FILEDESCRIPTORA for the image.
 * @return CFSTR_FILEDESCRIPTORA
 */
HGLOBAL DILDataObjectPrivate::getFileDescriptorA(void) const
{
	// Create a CFSTR_FILEDESCRIPTOR for a single virtual file.
	// NOTE: FILEGROUPDESCRIPTORA contains one FILEDESCRIPTORA.
	const size_t buf_size = sizeof(FILEGROUPDESCRIPTORA);
	HGLOBAL hglbFileDesc = GlobalAlloc(GMEM_MOVEABLE, buf_size);
	if (!hglbFileDesc) {
		return nullptr;
	}

	FILEGROUPDESCRIPTORA *const fileGroupDesc = static_cast<FILEGROUPDESCRIPTORA*>(GlobalLock(hglbFileDesc));
	if (!fileGroupDesc) {
		GlobalFree(hglbFileDesc);
		return nullptr;
	}

	fileGroupDesc->cItems = 1;

	FILEDESCRIPTORA *const fileDesc = &fileGroupDesc->fgd[0];
	fileDesc->dwFlags = FD_ATTRIBUTES;
	fileDesc->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	strcpy(fileDesc->cFileName, "test123.png");

	GlobalUnlock(hglbFileDesc);
	return hglbFileDesc;
}

/**
 * Get the image data in PNG format.
 * @return HGLOBAL containing the image data, or nullptr on error.
 */
HGLOBAL DILDataObjectPrivate::getFileContents(void) const
{
	// Save to a VectorFile.
	// The data will be copied to an HGLOBAL later.
	VectorFilePtr pngData(new VectorFile());

	// Save the image using RpPngWriter.
	// TODO: Generate a temporary filename, possibly based on game ID or source filename?
	unique_ptr<RpPngWriter> pngWriter;
	if (std::holds_alternative<IconAnimDataConstPtr>(imgData)) {
		// Animated icon
		pngWriter.reset(new RpPngWriter(pngData, std::get<IconAnimDataConstPtr>(imgData)));
	} else if (std::holds_alternative<rp_image_const_ptr>(imgData)) {
		// Standard icon
		pngWriter.reset(new RpPngWriter(pngData, std::get<rp_image_const_ptr>(imgData)));
	} else {
		// No icon...
		return nullptr;
	}

	/** tEXt chunks **/
	// TODO: Add text fields indicating the source game.
	RpPngWriter::kv_vector kv;

	// Software
	kv.emplace_back("Software", "ROM Properties Page shell extension (Win32)");

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR and IDAT **/
	int pwRet = pngWriter->write_IHDR();
	if (pwRet != 0) {
		// Error writing IHDR.
		return nullptr;
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing IDAT.
		return nullptr;
	}

	// Copy the VectorFile data to an HGLOBAL.
	const std::vector<uint8_t> &vec = pngData->vector();
	HGLOBAL hglbPngFile = GlobalAlloc(GMEM_MOVEABLE, vec.size());
	if (!hglbPngFile) {
		return nullptr;
	}

	uint8_t *const fileBuf = static_cast<uint8_t*>(GlobalLock(hglbPngFile));
	if (!fileBuf) {
		GlobalFree(hglbPngFile);
		return nullptr;
	}

	memcpy(fileBuf, vec.data(), vec.size());
	GlobalUnlock(hglbPngFile);
	return hglbPngFile;
}

/** DILDataObject **/

DILDataObject::DILDataObject(const LibRpTexture::rp_image_const_ptr &img)
	: d_ptr(new DILDataObjectPrivate(img))
{}

DILDataObject::DILDataObject(const LibRpBase::IconAnimDataConstPtr &iconAnimData)
	: d_ptr(new DILDataObjectPrivate(iconAnimData))
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

	switch (idx) {
		case 0:	// CFSTR_FILEDESCRIPTORW
			// Get the file descriptor.
			pmedium->hGlobal = d->getFileDescriptorW();
			break;

		case 1:	// CFSTR_FILEDESCRIPTORA
			// Get the file descriptor.
			pmedium->hGlobal = d->getFileDescriptorA();
			break;

		case 2:	// CFSTR_FILECONTENTS
			// Get the file contents.
			pmedium->hGlobal = d->getFileContents();
			if (!pmedium->hGlobal) {
				// Shouldn't happen...
				return E_FAIL;
			}
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
	RP_D(const DILDataObject);
	HRESULT hr = SHCreateStdEnumFmtEtc(
		static_cast<int>(d->formatEtc.size()),
		d->formatEtc.data(),
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
