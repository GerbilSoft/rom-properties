#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C

#include "RP_ComBase.hpp"

// CLSID
extern "C" {
	extern const GUID CLSID_RP_ExtractIcon;
	extern const wchar_t CLSID_RP_ExtractIcon_Str[];
}

// FIXME: Use this GUID for RP_ExtractIcon etc.
class __declspec(uuid("{E51BC107-E491-4B29-A6A3-2A4309259802}"))
RP_ExtractIcon : public RP_ComBase2<IExtractIcon, IPersistFile>
{
	public:
		RP_ExtractIcon();

	public:
		// IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override;

		// IExtractIcon
		STDMETHOD(GetIconLocation)(UINT uFlags, __out_ecount(cchMax) LPTSTR pszIconFile,
			UINT cchMax, __out int *piIndex, __out UINT *pwFlags) override;
		STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex,
			__out_opt HICON *phiconLarge,__out_opt HICON *phiconSmall,
			UINT nIconSize) override;

		// IPersist (IPersistFile base class)
		STDMETHOD(GetClassID)(__RPC__out CLSID *pClassID) override;
		// IPersistFile
		STDMETHOD(IsDirty)(void) override;
		STDMETHOD(Load)(__RPC__in LPCOLESTR pszFileName, DWORD dwMode) override;
		STDMETHOD(Save)(__RPC__in_opt LPCOLESTR pszFileName, BOOL fRemember) override;
		STDMETHOD(SaveCompleted)(__RPC__in_opt LPCOLESTR pszFileName) override;
		STDMETHOD(GetCurFile)(__RPC__deref_out_opt LPOLESTR *ppszFileName) override;
};

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__ */
