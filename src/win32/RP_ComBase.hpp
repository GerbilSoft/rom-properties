#ifndef __ROMPROPERTIES_WIN32_RP_COMBASE_H__
#define __ROMPROPERTIES_WIN32_RP_COMBASE_H__

/**
 * COM base class. Handles reference counting and IUnknown.
 * References:
 * - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
 * - http://www.codeproject.com/Articles/338268/COM-in-C
 * - http://stackoverflow.com/questions/17310733/how-do-i-re-use-an-interface-implementation-in-many-classes
 */

#if defined(__GNUC__) && defined(__MINGW32__)
/**
 * MinGw-w64 is also missing ULONG overloads for the various
 * atomic functions.
 */
static inline ULONG InterlockedIncrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedIncrement(reinterpret_cast<LONG volatile*>(Addend)));
}
static inline ULONG InterlockedDecrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedDecrement(reinterpret_cast<LONG volatile*>(Addend)));
}
#endif /* __GNUC__ && __MINGW32__ */

#ifdef _MSC_VER
#define UUID_ATTR(str) __declspec(uuid(str))
#else /* !_MSC_VER */
// UUID attribute is not supported by gcc-5.2.0.
#define UUID_ATTR(str)
#endif /* _MSC_VER */

#define UNUSED(x) ((void)x)

// References of all objects.
extern volatile ULONG RP_ulTotalRefCount;

/**
 * Is an RP_ComBase object referenced?
 * @return True if RP_ulTotalRefCount > 0; false if not.
 */
static inline bool RP_ComBase_isReferenced(void)
{
	return (RP_ulTotalRefCount > 0);
}

#define RP_COMBASE_IMPL(name) \
{ \
	protected: \
		/* References of this object. */ \
		volatile ULONG m_ulRefCount; \
	\
	public: \
		name() : m_ulRefCount(0) { } \
		virtual ~name() { } \
	\
	public: \
		/** IUnknown **/ \
		STDMETHOD_(ULONG,AddRef)(void) override \
		{ \
			InterlockedIncrement(&RP_ulTotalRefCount); \
			InterlockedIncrement(&m_ulRefCount); \
			return m_ulRefCount; \
		} \
		\
		STDMETHOD_(ULONG,Release)(void) override \
		{ \
			ULONG ulRefCount = InterlockedDecrement(&m_ulRefCount); \
			if (ulRefCount == 0) { \
				/* No more references. */ \
				delete this; \
			} \
			\
			InterlockedDecrement(&RP_ulTotalRefCount); \
			return ulRefCount; \
		} \
}

template<class I>
class RP_ComBase : public I
	RP_COMBASE_IMPL(RP_ComBase);

template<class I1, class I2>
class RP_ComBase2 : public I1, public I2
	RP_COMBASE_IMPL(RP_ComBase2);

#endif /* __ROMPROPERTIES_WIN32_RP_COMBASE_H__ */
