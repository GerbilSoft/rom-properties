#ifndef __ROMPROPERTIES_WIN32_RP_CLASSFACTORY_H__
#define __ROMPROPERTIES_WIN32_RP_CLASSFACTORY_H__

// References:
// - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
// - http://www.codeproject.com/Articles/338268/COM-in-C

#include "RP_ComBase.hpp"

template<class comObj>
class RP_MultiCreator
{
	protected:
		comObj *CreateObject()
		{
			return new comObj;
		}
};

template <class comObj, class creatorClass = RP_MultiCreator<comObj> >
class RP_ClassFactory : public RP_ComBase<IClassFactory>, public creatorClass
{
	public:
		RP_ClassFactory() { }

	public:
		/** IUnknown **/

		#if 0
		// FIXME: This caused crashes. Try implementing it again later?
		// Call the RP_ComBase functions.
		// References:
		// - http://stackoverflow.com/questions/5478669/good-techniques-for-keeping-com-classes-that-implement-multiple-interfaces-manag
		// - http://stackoverflow.com/a/5480348
		STDMETHOD_(ULONG, AddRef)(void) { return RP_ComBase::AddRef(); }
		STDMETHOD_(ULONG, Release)(void) { return RP_ComBase::Release(); }
		#endif

		STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override
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
			if (riid == IID_IUnknown || riid == IID_IClassFactory) {
				*ppvObj = static_cast<IClassFactory*>(this);
			} else {
				// Interface is not supported.
				return E_NOINTERFACE;
			}

			// Make sure we count this reference.
			AddRef();
			return NOERROR;
		}

		/** IClassFactory **/

		STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject) override
		{
			// Always set out parameter to NULL, validating it first.
			if (!ppvObject)
				return E_INVALIDARG;
			*ppvObject = NULL;

			if (pUnkOuter) {
				// Aggregation is not supported.
				return CLASS_E_NOAGGREGATION;
			}

			// Create an instance of the object.
			comObj *pObj = RP_MultiCreator<comObj>::CreateObject();
			if (!pObj) {
				// Could not create the object.
				return E_OUTOFMEMORY;
			}

			// Query the object for the requested interface.
			HRESULT hr = pObj->QueryInterface(riid, ppvObject);
			pObj->Release();
			return hr;
		}

		STDMETHOD(LockServer)(BOOL fLock) override
		{
			// Not implemented.
			UNUSED(fLock);
			return S_OK;
		}
};

#endif /* __ROMPROPERTIES_WIN32_RP_CLASSFACTORY_H__ */
