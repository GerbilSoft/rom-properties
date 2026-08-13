/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DILDataObject.hpp: IDataObject implementation for DragImageLabel.       *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://www.catch22.net/tuts/ole/implementing-idataobject/

#pragma once

#include "common.h"

// Other rom-properties libraries
#include "librpbase/img/IconAnimData.hpp"
#include "librptexture/img/rp_image.hpp"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

class DILDataObjectPrivate;

class DILDataObject final : public LibWin32Common::ComBase<IDataObject, IDropSource>
{
public:
	explicit DILDataObject(const LibRpTexture::rp_image_const_ptr &img);
	explicit DILDataObject(const LibRpBase::IconAnimDataConstPtr &iconAnimData);
protected:
	~DILDataObject() final;

private:
	typedef LibWin32Common::ComBase<IDataObject, IDropSource> super;
	friend class DILDataObjectPrivate;
	DILDataObjectPrivate *const d_ptr;
public:
	RP_DISABLE_COPY(DILDataObject)

public:
	// DILDataObject-specific (non-COM)

	/**
	 * Set the filename for the dropped object.
	 * @param filename Filename (or nullptr to clear the filename)
	 */
	void setFilename(LPCTSTR filename);

	/**
	 * Set the mtime for the dropped object.
	 * @param mtime Modification time (last write time) (or nullptr to clear the mtime)
	 */
	void setMTime(const FILETIME *mtime);

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj) final;

	// IDataObject
	IFACEMETHODIMP GetData(_In_ FORMATETC *pformatetcIn, _Out_ STGMEDIUM *pmedium) final;
	IFACEMETHODIMP GetDataHere(_In_ FORMATETC *pformatetc, _Inout_ STGMEDIUM *pmedium) final;
	IFACEMETHODIMP QueryGetData(__RPC__in_opt FORMATETC *pformatetc) final;
	IFACEMETHODIMP GetCanonicalFormatEtc(__RPC__in_opt FORMATETC *pformatetcIn, __RPC__out FORMATETC *pformatetcOut) final;
	IFACEMETHODIMP SetData(_In_ FORMATETC *pformatetc, _In_ STGMEDIUM *pmedium, BOOL fRelease) final;
	IFACEMETHODIMP EnumFormatEtc(DWORD dwDirection, __RPC__deref_out_opt IEnumFORMATETC **ppenumFormatEtc) final;
	IFACEMETHODIMP DAdvise(__RPC__in FORMATETC *pformatetc, DWORD advf, __RPC__in_opt IAdviseSink *pAdvSink, __RPC__out DWORD *pdwConnection) final;
	IFACEMETHODIMP DUnadvise(DWORD dwConnection) final;
	IFACEMETHODIMP EnumDAdvise(__RPC__deref_out_opt IEnumSTATDATA **ppenumAdvise) final;

	// IDropSource
	IFACEMETHODIMP QueryContinueDrag(_In_ BOOL fEscapePressed, _In_ DWORD grfKeyState) final;
	IFACEMETHODIMP GiveFeedback(_In_ DWORD dwEffect) final;
};
