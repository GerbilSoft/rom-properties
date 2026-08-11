/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DILDataObject.cpp: IDataObject implementation for DragImageLabel.       *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://www.catch22.net/tuts/ole/implementing-idataobject/
#include "DILDataObject.hpp"
#include "RpImageWin32.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPngWriter.hpp"
#include "librpfile/VectorFile.hpp"
#include "librptext/wchar.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpTexture;

// libwin32ui
#include "libwin32ui/WinUI.hpp"
#include <uxtheme.h>	// for IsThemeActive()

// C includes
#include "tcharx.h"

// C++ STL classes
#include <memory>
#include <vector>
using std::array;
using std::string;
using std::tstring;
using std::unique_ptr;
using std::vector;

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
	~DILDataObjectPrivate();

private:
	/**
	 * Initialize the data vectors.
	 */
	void initDataVectors(void);

public:
	std::variant<std::monostate, rp_image_const_ptr, IconAnimDataConstPtr> imgData;

public:
	// Our own data takes up the first 3 indexes in vec_formatEtc and vec_stgMedium.
	static constexpr int OUR_DATA_COUNT = 3;

	// Data formats and storage
	vector<FORMATETC> vec_formatEtc;
	vector<STGMEDIUM> vec_stgMedium;

	// Filename for the dropped object
	tstring filename;

	/**
	 * Check if a format is supported.
	 * @param pformatetc FORMATETC
	 * @return Index if supported; -1 if not.
	 */
	int lookupFormatEtc(_In_ const FORMATETC *pformatetc) const;

	/**
	 * dup() an HGLOBAL.
	 * @param hMem HGLOBAL
	 * @return dup()'d HGLOBAL
	 */
	static HGLOBAL dupGlobalMem(HGLOBAL hMem);

public:
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
	 * Convert the loaded rp_image or IconAnimData to a .PNG file and
	 * store it in the CFSTR_FILECONTENTS HGLOBAL in vec_stgMedium.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int convertImageToHGLOBAL(void);
};

/** DILDataObjectPrivate **/

DILDataObjectPrivate::DILDataObjectPrivate(const rp_image_const_ptr &img)
	: imgData(img)
{
	initDataVectors();
}

DILDataObjectPrivate::DILDataObjectPrivate(const IconAnimDataConstPtr &iconAnimData)
	: imgData(iconAnimData)
{
	initDataVectors();
}

DILDataObjectPrivate::~DILDataObjectPrivate()
{
	// Make sure any data set via SetData() is freed.
	// Also CFSTR_FILECONTENTS.
	for (int i = 2; i < static_cast<int>(vec_stgMedium.size()); i++) {
		ReleaseStgMedium(&vec_stgMedium[i]);
	}
}

/**
 * Initialize the data vectors.
 */
void DILDataObjectPrivate::initDataVectors(void)
{
	// Initialize vec_formatEtc and vec_stgMedium.
	// NOTE: Cannot make it static const because CFSTR_* formats have to be
	// converted to integers using RegisterClipboardFormat().
	vec_formatEtc.reserve(OUR_DATA_COUNT);
	vec_stgMedium.reserve(OUR_DATA_COUNT);

	FORMATETC fmtetc = {0, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	static const STGMEDIUM stgm_hGlobal = {TYMED_HGLOBAL, nullptr, nullptr};
	static const STGMEDIUM stgm_null = {TYMED_NULL, nullptr, nullptr};

	// CFSTR_FILEDESCRIPTORW
	fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
	vec_formatEtc.push_back(fmtetc);
	vec_stgMedium.push_back(stgm_hGlobal);

	// CFSTR_FILEDESCRIPTORA
	fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
	vec_formatEtc.push_back(fmtetc);
	vec_stgMedium.push_back(stgm_hGlobal);

	// CFSTR_FILECONTENTS
	fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILECONTENTS);
	vec_formatEtc.push_back(fmtetc);
	vec_stgMedium.push_back(stgm_null);

	assert(vec_formatEtc.size() == static_cast<size_t>(OUR_DATA_COUNT));
	assert(vec_stgMedium.size() == static_cast<size_t>(OUR_DATA_COUNT));
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

	for (int i = 0; i < static_cast<int>(vec_formatEtc.size()); i++) {
		const FORMATETC *const fmtchk = &vec_formatEtc[i];
		if (((fmtchk->tymed & pformatetc->tymed) != 0) &&
		    fmtchk->cfFormat == pformatetc->cfFormat &&
		    fmtchk->dwAspect == pformatetc->dwAspect)
		{
			// Found a matching format.
			return i;
		}
	}

	// Not found.
	return -1;
}

/**
 * dup() an HGLOBAL.
 * @param hMem HGLOBAL
 * @return dup()'d HGLOBAL
 */
HGLOBAL DILDataObjectPrivate::dupGlobalMem(HGLOBAL hMem)
{
	const SIZE_T len = GlobalSize(hMem);
	HGLOBAL hRet = GlobalAlloc(GMEM_MOVEABLE, len);

	PVOID source = GlobalLock(hMem);
	if (!source) {
		GlobalFree(hRet);
		return nullptr;
	}
	PVOID dest = GlobalLock(hRet);
	if (!dest) {
		GlobalUnlock(hMem);
		GlobalFree(hRet);
		return nullptr;
	}

	memcpy(dest, source, len);

	GlobalUnlock(hRet);
	GlobalUnlock(hMem);
	return hRet;
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

	// If the CFSTR_FILECONTENTS HGLOBAL hasn't been initialized yet,
	// initialize it now.
	assert(vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT));
	if (vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT)) {
		if (!vec_stgMedium[2].hGlobal) {
			const_cast<DILDataObjectPrivate*>(this)->convertImageToHGLOBAL();
		}
	}

	fileGroupDesc->cItems = 1;

	FILEDESCRIPTORW *const fileDesc = &fileGroupDesc->fgd[0];
	fileDesc->dwFlags = FD_ATTRIBUTES;
	fileDesc->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	// Get the HGLOBAL size.
	if (vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT)) {
		if (vec_stgMedium[2].hGlobal) {
			const SIZE_T len = GlobalSize(vec_stgMedium[2].hGlobal);
			if (len > 0) {
				fileDesc->nFileSizeLow = (len & 0xFFFFFFFFU);
				// NOTE: The PNG should never be more than 4 GB.
				// Hard-coding this to 0 to prevent warnings in 32-bit builds.
				fileDesc->nFileSizeHigh = 0; //(len >> 32);
				fileDesc->dwFlags |= FD_FILESIZE;
			}
		}
	}

#ifdef UNICODE
	wcsncpy_s(fileDesc->cFileName, _countof(fileDesc->cFileName), filename.c_str(), _TRUNCATE);
#else /* !UNICODE */
	const wstring wstr = A2W_s(filename);
	wcsncpy_s(fileDesc->cFileName, _countof(fileDesc->cFileName), wstr.c_str(), _TRUNCATE);
#endif /* UNICODE */

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

	// If the CFSTR_FILECONTENTS HGLOBAL hasn't been initialized yet,
	// initialize it now.
	assert(vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT));
	if (vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT)) {
		if (!vec_stgMedium[2].hGlobal) {
			const_cast<DILDataObjectPrivate*>(this)->convertImageToHGLOBAL();
		}
	}

	fileGroupDesc->cItems = 1;

	FILEDESCRIPTORA *const fileDesc = &fileGroupDesc->fgd[0];
	fileDesc->dwFlags = FD_ATTRIBUTES;
	fileDesc->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	// Get the HGLOBAL size.
	if (vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT)) {
		if (vec_stgMedium[2].hGlobal) {
			const SIZE_T len = GlobalSize(vec_stgMedium[2].hGlobal);
			if (len > 0) {
				fileDesc->nFileSizeLow = (len & 0xFFFFFFFFU);
				// NOTE: The PNG should never be more than 4 GB.
				// Hard-coding this to 0 to prevent warnings in 32-bit builds.
				fileDesc->nFileSizeHigh = 0; //(len >> 32);
				fileDesc->dwFlags |= FD_FILESIZE;
			}
		}
	}

	// TODO: Timestamp and other attributes?

#ifdef UNICODE
	const string str = W2A(filename);
	strncpy_s(fileDesc->cFileName, _countof(fileDesc->cFileName), str.c_str(), _TRUNCATE);
#else /* !UNICODE */
	strncpy_s(fileDesc->cFileName, _countof(fileDesc->cFileName), filename.c_str(), _TRUNCATE);
#endif /* UNICODE */

	GlobalUnlock(hglbFileDesc);
	return hglbFileDesc;
}

/**
 * Convert the loaded rp_image or IconAnimData to a .PNG file and
 * store it in the CFSTR_FILECONTENTS HGLOBAL in vec_stgMedium.
 * @return 0 on success; negative POSIX error code on error.
 */
int DILDataObjectPrivate::convertImageToHGLOBAL(void)
{
	assert(vec_stgMedium.size() >= static_cast<size_t>(OUR_DATA_COUNT));
	if (vec_stgMedium.size() < static_cast<size_t>(OUR_DATA_COUNT)) {
		// The vectors haven't been initialized yet...
		return -ENODATA;
	}

	// CFSTR_FILECONTENTS is stored in index 2.
	STGMEDIUM *const stgm = &vec_stgMedium[2];
	if (stgm->hGlobal) {
		// Delete the existing HGLOBAL.
		GlobalFree(stgm->hGlobal);
		stgm->tymed = TYMED_NULL;
		stgm->hGlobal = nullptr;
	}

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
		return -ENOENT;
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
		return -EIO;
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing IDAT.
		return -EIO;
	}

	// Copy the VectorFile data to an HGLOBAL.
	const std::vector<uint8_t> &vec = pngData->vector();
	HGLOBAL hglbPngFile = GlobalAlloc(GMEM_MOVEABLE, vec.size());
	if (!hglbPngFile) {
		return -ENOMEM;
	}

	uint8_t *const fileBuf = static_cast<uint8_t*>(GlobalLock(hglbPngFile));
	if (!fileBuf) {
		GlobalFree(hglbPngFile);
		return -ENOMEM;
	}

	memcpy(fileBuf, vec.data(), vec.size());
	GlobalUnlock(hglbPngFile);
	stgm->tymed = TYMED_HGLOBAL;
	stgm->hGlobal = hglbPngFile;
	return 0;
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

/**
 * Set the filename for the dropped object.
 * @param filename Filename (or nullptr to clear the filename)
 */
void DILDataObject::setFilename(LPCTSTR filename)
{
	RP_D(DILDataObject);
	if (filename) {
		d->filename = filename;
	} else {
		d->filename.clear();
	}
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
	RP_D(DILDataObject);
	int idx = d->lookupFormatEtc(pformatetcIn);
	if (idx < 0) {
		// Not supported...
		return DV_E_FORMATETC;
	}

	// found a match - transfer data into supplied storage medium
	const STGMEDIUM *const stgm = &d->vec_stgMedium[idx];
	pmedium->tymed = stgm->tymed;
	pmedium->pUnkForRelease = stgm->pUnkForRelease;

	// TODO: Cache the hGlobal data in d->vec_stgMedium?
	// For now, d->vec_stgMedium data is only valid for anything set with SetData().
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
			// Make sure the image has been converted.
			if (!d->vec_stgMedium[2].hGlobal) {
				d->convertImageToHGLOBAL();
			}
			// fall-through

		default:
			// Get the STGMEDIUM that was previously set using SetData().
			// Also CFSTR_FILECONTENTS.
			// TODO: Verify actions using: https://learn.microsoft.com/en-us/windows/win32/api/ole2/nf-ole2-releasestgmedium
			switch (stgm->tymed) {
				default:
					assert(!"Unsupported TYMED!");
					return DV_E_TYMED;

				case TYMED_NULL:
					// Nothing to do here...
					break;

				case TYMED_HGLOBAL:
					if (stgm->pUnkForRelease) {
						// Return the HGLOBAL as-is.
						// TODO: Verify that this is correct.
						pmedium->hGlobal = stgm->hGlobal;
					} else {
						// Need to dup() the HGLOBAL.
						pmedium->hGlobal = (stgm->hGlobal != nullptr)
							? d->dupGlobalMem(stgm->hGlobal)
							: nullptr;
					}
					break;

				case TYMED_ISTREAM:
					if (stgm->pstm) {
						stgm->pstm->AddRef();
					}
					pmedium->pstm = stgm->pstm;
					break;

				case TYMED_ISTORAGE:
					if (stgm->pstg) {
						stgm->pstg->AddRef();
					}
					pmedium->pstg = stgm->pstg;
					break;

				case TYMED_FILE:
				case TYMED_GDI:
				case TYMED_MFPICT:
				case TYMED_ENHMF:
					assert(!"Unsupported TYMED!");
					return DV_E_TYMED;
			}
			break;
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
	if (!pformatetc || !pmedium) {
		return E_POINTER;
	}

	// Check if the FORMATETC already exists.
	// If it does, and it's after our default data, replace it.
	// Otherwise, add it.
	RP_D(DILDataObject);
	int idx = d->lookupFormatEtc(pformatetc);
	if (idx >= d->OUR_DATA_COUNT) {
		// Free the existing STGMEDIUM.
		ReleaseStgMedium(&d->vec_stgMedium[idx]);
		memcpy(&d->vec_formatEtc[idx], pformatetc, sizeof(FORMATETC));
		memcpy(&d->vec_stgMedium[idx], pmedium, sizeof(STGMEDIUM));
	} else if (idx < 0) {
		// Not found. Create a new entry.
		idx = static_cast<int>(d->vec_formatEtc.size());
		d->vec_formatEtc.push_back(*pformatetc);
		d->vec_stgMedium.push_back(*pmedium);
	} else {
		// Matches one of our predefined data entries...
		return DV_E_LINDEX;
	}

	if (!fRelease) {
		// fRelease == false: ownership is *not* transferred to us.
		// We'll need to dup() the input.
		// NOTE: CopyStgMedium() in urlmon.lib can handle this...
		STGMEDIUM *const stgm = &d->vec_stgMedium[idx];
		switch (stgm->tymed) {
			default:
				// Unsupported...
				// TODO: Undo the vector changes?
				assert(!"Unsupported TYMED!");
				stgm->tymed = TYMED_NULL;
				return DV_E_TYMED;

			case TYMED_NULL:
				// Nothing to do here...
				break;

			case TYMED_HGLOBAL:
				stgm->hGlobal = (pmedium->hGlobal != nullptr)
					? d->dupGlobalMem(pmedium->hGlobal)
					: nullptr;
				break;

			case TYMED_FILE:
				// from wine's urlmon_main.c
				if (pmedium->lpszFileName && !pmedium->pUnkForRelease) {
					size_t cb = (wcslen(pmedium->lpszFileName) + 1) * sizeof(wchar_t);
					// TODO: Return E_OUTOFMEMORY if this fails. (TODO: Undo the vector changes?)
					stgm->lpszFileName = static_cast<LPOLESTR>(CoTaskMemAlloc(cb));
					memcpy(stgm->lpszFileName, pmedium->lpszFileName, cb);
				} else {
					stgm->lpszFileName = pmedium->lpszFileName;
				}
				break;

			case TYMED_ISTREAM:
				stgm->pstm->AddRef();
				break;

			case TYMED_ISTORAGE:
				stgm->pstg->AddRef();
				break;

			case TYMED_GDI:
			case TYMED_MFPICT:
			case TYMED_ENHMF:
				// TODO: Undo the vector changes?
				assert(!"Unsupported TYMED!");
				stgm->tymed = TYMED_NULL;
				return DV_E_TYMED;
		}
	}

	return S_OK;
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
		static_cast<int>(d->vec_formatEtc.size()),
		d->vec_formatEtc.data(),
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
